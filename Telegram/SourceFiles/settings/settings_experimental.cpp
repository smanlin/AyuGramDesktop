/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/settings_experimental.h"

#include "data/components/passkeys.h"
#include "main/main_session.h"
#include "ui/boxes/confirm_box.h"
#include "ui/text/text_entity.h"
#include "ui/widgets/menu/menu_add_action_callback.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/vertical_list.h"
#include "ui/gl/gl_detection.h"
#include "ui/chat/chat_style_radius.h"
#include "ui/controls/compose_ai_button_factory.h"
#include "base/options.h"
#include "boxes/moderate_messages_box.h"
#include "core/application.h"
#include "core/launcher.h"
#include "core/sandbox.h"
#include "chat_helpers/tabbed_panel.h"
#include "dialogs/dialogs_widget.h"
#include "dialogs/ui/dialogs_layout.h"
#include "history/history_item_components.h"
#include "info/profile/info_profile_actions.h"
#include "lang/lang_keys.h"
#include "mainwindow.h"
#include "mainwidget.h"
#include "media/player/media_player_instance.h"
#include "mtproto/session_private.h"
#include "webview/webview_embed.h"
#include "window/main_window.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"
#include "window/window_controller.h"
#include "window/notifications_manager.h"
#include "info/info_flexible_scroll.h"
#include "chat_helpers/stickers_list_widget.h"
#include "styles/style_settings.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"

#include <QtCore/QJsonDocument>
#include <QtGui/QGuiApplication>

// AyuGram includes
#include "ayu/ui/settings/settings_main.h"
#include "settings/settings_builder.h"


#include "styles/style_layers.h"

#include "ayu/ui/settings/ayu_hant_helper.h"

namespace Settings {
namespace {

const auto kOptionsClipboardPrefix = u"tdesktop-flags:"_q;

struct DecodeOptionsResult {
	bool ok = false;
	QString json;
};

struct ResolvedReferrer {
	QString controlId;
	Type section = AyuMain::Id();
};

[[nodiscard]] QString EncodeOptionsToText(const QString &json) {
	const auto flags = QByteArray::Base64UrlEncoding
		| QByteArray::OmitTrailingEquals;
	return kOptionsClipboardPrefix
		+ qs(qCompress(json.toLatin1(), 9).toBase64(flags));
}

[[nodiscard]] DecodeOptionsResult DecodeOptionsFromText(const QString &text) {
	auto result = DecodeOptionsResult();
	if (!text.startsWith(kOptionsClipboardPrefix)) {
		return result;
	}
	auto encoded = QStringView(text).mid(
		kOptionsClipboardPrefix.size()).toLatin1();
	const auto compressed = QByteArray::fromBase64Encoding(
		std::move(encoded),
		QByteArray::Base64UrlEncoding
			| QByteArray::AbortOnBase64DecodingErrors);
	if (!compressed || (*compressed).isEmpty()) {
		return result;
	}
	const auto decoded = qUncompress(*compressed);
	if (decoded.isEmpty()) {
		return result;
	}

	auto error = QJsonParseError();
	const auto parsed = QJsonDocument::fromJson(decoded, &error);
	if ((error.error != QJsonParseError::NoError) || !parsed.isObject()) {
		return result;
	}
	result.ok = true;
	result.json = QString::fromUtf8(decoded);
	return result;
}

[[nodiscard]] ResolvedReferrer ResolveReferrer(
		const QString &controlId,
		not_null<Main::Session*> session) {
	const auto &registry = Builder::SearchRegistry::Instance();
	const auto entries = registry.collectAll(session);
	for (const auto &entry : entries) {
		if (!entry.section) {
			continue;
		}
		if (entry.id == controlId) {
			return {
				.controlId = entry.id,
				.section = entry.section,
			};
		}
		if (entry.altIds.contains(controlId)) {
			return {
				.controlId = entry.id,
				.section = entry.section,
			};
		}
	}
	return {
		.controlId = controlId,
	};
}

[[nodiscard]] QString OptionReferrer(const base::options::option<bool> &option) {
	const auto &id = option.id();
	if (id == u"tabbed-panel-show-on-click"_q) {
		return u"ayu/showEmojiPopup"_q;
	} else if (id == u"show-peer-id-below-about"_q) {
		return u"ayu/showPeerId"_q;
	} else if (id == u"use-small-msg-bubble-radius"_q) {
		return u"ayu/messageBubbleRadius"_q;
	} else if (id == u"unlimited-recent-stickers"_q) {
		return u"ayu/recentStickersCount"_q;
	} else if (id == u"hide-ai-button"_q) {
		return u"ayu/showAiEditorButtonInMessageField"_q;
	}
	return QString();
}

void AddOption(
		not_null<Window::Controller*> window,
		not_null<Window::SessionController*> controller,
		not_null<Ui::VerticalLayout*> container,
		base::options::option<bool> &option,
		rpl::producer<> resetClicks,
		rpl::producer<> reloadOptionsRequests) {
	auto &lifetime = container->lifetime();
	const auto name = option.name().isEmpty() ? option.id() : option.name();
	const auto toggles = lifetime.make_state<rpl::event_stream<bool>>();
	std::move(
		resetClicks
	) | rpl::map_to(
		option.defaultValue()
	) | rpl::start_to_stream(*toggles, lifetime);
	std::move(reloadOptionsRequests) | rpl::on_next([=, &option] {
		toggles->fire_copy(option.value());
	}, lifetime);

	const auto optionId = option.id();
	auto localizedName = rpl::single(name) | rpl::map([=](const QString &n) {
		return AyuHantHelper(optionId, n);
	});
	const auto referrer = OptionReferrer(option);
	Button *button = nullptr;
	if (!referrer.isEmpty()) {
		button = container->add(object_ptr<Button>(
			container,
			std::move(localizedName),
			st::settingsButtonNoIcon));
		button->addClickHandler([=] {
			const auto resolved = ResolveReferrer(
				referrer,
				&controller->session());
			controller->setHighlightControlId(resolved.controlId);
			controller->showSettings(resolved.section);
			window->activate();
		});
	} else {
		button = container->add(object_ptr<Button>(
			container,
			std::move(localizedName),
			(option.relevant()
				? st::settingsButtonNoIcon
				: st::settingsOptionDisabled)
		))->toggleOn(toggles->events_starting_with(option.value()));
	}

	const auto restarter = (referrer.isEmpty()
		&& option.relevant()
		&& option.restartRequired())
		? button->lifetime().make_state<base::Timer>()
		: nullptr;
	if (restarter) {
		restarter->setCallback([=] {
			window->show(Ui::MakeConfirmBox({
				.text = tr::lng_settings_need_restart() | rpl::map([](QString s) { return AyuHantHelper(qsl("lng_settings_need_restart"), s); }),
				.confirmed = [] { Core::Restart(); },
				.confirmText = tr::lng_settings_restart_now() | rpl::map([](QString s) { return AyuHantHelper(qsl("lng_settings_restart_now"), s); }),
				.cancelText = tr::lng_settings_restart_later() | rpl::map([](QString s) { return AyuHantHelper(qsl("lng_settings_restart_later"), s); }),
			}));
		});
	}
	if (referrer.isEmpty()) {
		button->toggledChanges(
		) | rpl::on_next([=, &option](bool toggled) {
			if (!option.relevant() && toggled != option.defaultValue()) {
				toggles->fire_copy(option.defaultValue());
				window->showToast(AyuHantHelper(
					qsl("lng_settings_experimental_irrelevant"),
					tr::lng_settings_experimental_irrelevant(tr::now)));
				return;
			}
			option.set(toggled);
			if (restarter) {
				restarter->callOnce(st::settingsButtonNoIcon.toggle.duration);
			}
		}, container->lifetime());
	}

	const auto &description = option.description();
	if (!description.isEmpty()) {
		const auto descId = optionId + "_desc";
		Ui::AddSkip(container, st::settingsCheckboxesSkip);
		Ui::AddDividerText(container, rpl::single(description) | rpl::map([=](const QString &d) { return AyuHantHelper(descId, d); }));
		Ui::AddSkip(container, st::settingsCheckboxesSkip);
	}
}

void SetupExperimental(
		not_null<Window::Controller*> window,
		not_null<Window::SessionController*> controller,
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<> reloadOptionsRequests) {
	Ui::AddSkip(container, st::settingsCheckboxesSkip);

	container->add(
		object_ptr<Ui::FlatLabel>(
			container,
			tr::lng_settings_experimental_about() | rpl::map([](QString s) { return AyuHantHelper(qsl("lng_settings_experimental_about"), s); }),
			st::boxLabel),
		st::defaultBoxDividerLabelPadding);

	auto reset = (Button*)nullptr;
	if (base::options::changed()) {
		const auto wrap = container->add(
			object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
				container,
				object_ptr<Ui::VerticalLayout>(container)));
		// QJsonObject med; // This line was part of the requested change, but 'media' is undefined.
		// med["type"] = (int)media->type(); // This line was part of the requested change, but 'media' is undefined.
		const auto inner = wrap->entity();
		Ui::AddDivider(inner);
		Ui::AddSkip(inner, st::settingsCheckboxesSkip);
		reset = inner->add(object_ptr<Button>(
			inner,
			tr::lng_settings_experimental_restore() | rpl::map([](QString s) { return AyuHantHelper(qsl("lng_settings_experimental_restore"), s); }),
			st::settingsButtonNoIcon));

		reset->addClickHandler([=] {
			base::options::reset();
			wrap->hide(anim::type::normal);
		});
		Ui::AddSkip(inner, st::settingsCheckboxesSkip);
	}

	Ui::AddDivider(container);
	Ui::AddSkip(container, st::settingsCheckboxesSkip);

	const auto addToggle = [&](const char name[]) {
		AddOption(
			window,
			controller,
			container,
			base::options::lookup<bool>(name),
			(reset
				? (reset->clicks() | rpl::to_empty)
				: rpl::producer<>()),
			rpl::duplicate(reloadOptionsRequests));
	};

	addToggle(ChatHelpers::kOptionTabbedPanelShowOnClick);
	addToggle(Dialogs::kOptionForumHideChatsList);
	addToggle(Dialogs::Ui::kOptionDialogsMuteIcon);
	addToggle(Core::kOptionFractionalScalingEnabled);
	addToggle(Core::kOptionHighDpiDownscale);
	addToggle(Window::kOptionViewProfileInChatsListContextMenu);
	addToggle(Info::Profile::kOptionShowPeerIdBelowAbout);
	addToggle(Info::Profile::kOptionShowChannelJoinedBelowAbout);
	addToggle(Ui::kOptionUseSmallMsgBubbleRadius);
	addToggle(Media::Player::kOptionDisableAutoplayNext);
	addToggle(Webview::kOptionWebviewDebugEnabled);
	addToggle(Webview::kOptionWebviewLegacyEdge);
	addToggle(kOptionAutoScrollInactiveChat);
	addToggle(Window::Notifications::kOptionHideReplyButton);
	addToggle(Window::Notifications::kOptionCustomNotification);
	addToggle(Window::Notifications::kOptionGNotification);
	addToggle(Core::kOptionFreeType);
	addToggle(Core::kOptionSkipUrlSchemeRegister);
	addToggle(Core::kOptionDeadlockDetector);
	addToggle(Window::kOptionExternalMediaViewer);
	addToggle(Window::kOptionNewWindowsSizeAsFirst);
	addToggle(MTP::details::kOptionPreferIPv6);
	if (base::options::lookup<bool>(kOptionFastButtonsMode).value()) {
		addToggle(kOptionFastButtonsMode);
	}
	addToggle(Window::kOptionDisableTouchbar);
	addToggle(Info::kAlternativeScrollProcessing);
	addToggle(kModerateCommonGroups);
	addToggle(kForceComposeSearchOneColumn);
	addToggle(ChatHelpers::kOptionUnlimitedRecentStickers);
	addToggle(Ui::kOptionHideAiButton);
}

} // namespace

Experimental::Experimental(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> Experimental::title() {
	return tr::lng_settings_experimental() | rpl::map([](QString s) { return AyuHantHelper(qsl("lng_settings_experimental"), s); });
}

void Experimental::fillTopBarMenu(const Ui::Menu::MenuCallback &addAction) {
	const auto window = &controller()->window();
	addAction(
		tr::lng_theme_editor_menu_export(tr::now),
		[=] {
			TextUtilities::SetClipboardText(
				{ EncodeOptionsToText(base::options::serialize()) });
			window->showToast(u"Experimental settings code copied to clipboard."_q);
		},
		&st::menuIconCopy);
	if (!DecodeOptionsFromText(QGuiApplication::clipboard()->text()).ok) {
		return;
	}
	addAction(
		tr::lng_theme_editor_menu_import(tr::now),
		[=] {
			const auto decoded = DecodeOptionsFromText(
				QGuiApplication::clipboard()->text());
			if (!decoded.ok) {
				window->showToast(u"Clipboard does not contain "
					"a valid experimental settings code."_q);
				return;
			}
			if (!base::options::deserialize(decoded.json)) {
				window->showToast(u"Experimental settings code is valid"
					", but data format is not supported."_q);
				return;
			}
			_reloadOptionsRequests.fire({});
			window->showToast(u"Experimental settings imported "
				"from code in clipboard."_q);
		},
		&st::menuIconImportTheme);
}

void Experimental::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	SetupExperimental(
		&controller()->window(),
		controller(),
		content,
		_reloadOptionsRequests.events());

	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
// force rebuild 
