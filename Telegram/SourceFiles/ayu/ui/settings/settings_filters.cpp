// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_filters.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/data/ayu_database.h"
#include "ayu/features/filters/filters_cache_controller.h"
#include "ayu/ui/boxes/import_filters_box.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/ayu_hant_helper.h"
#include "ayu/ui/settings/settings_main.h"
#include "ayu/utils/telegram_helpers.h"
#include "boxes/abstract_box.h"
#include "boxes/peer_list_box.h"
#include "core/application.h"
#include "filters/per_dialog_filter.h"
#include "filters/settings_filters_list.h"
#include "inline_bots/bot_attach_web_view.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_boxes.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/vertical_list.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/menu/menu_add_action_callback.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"
#include <QRegularExpression>

namespace Settings {

using namespace Builder;
using namespace AyBuilder;

namespace {

void BuildFiltersSettings(SectionBuilder &builder) {
	auto *settings = &AyuSettings::getInstance();

	builder.addSkip();
	builder.addSubsectionTitle(AYU_T(ayu_RegexFilters));

	const auto enabledButton = builder.addButton({
		.id = u"ayu/filtersEnabled"_q,
		.title = AYU_T(ayu_RegexFiltersEnable),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filtersEnabled()),
	});
	if (enabledButton) {
		enabledButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->filtersEnabled());
		}) | on_next([=](bool enabled) {
			AyuSettings::getInstance().setFiltersEnabled(enabled);
			FiltersCacheController::rebuildCache();
			FiltersCacheController::fireUpdate();
		}, enabledButton->lifetime());
	}

	const auto sharedButton = builder.addButton({
		.id = u"ayu/filtersEnabledInChats"_q,
		.altIds = { u"ayu/filtersInChats"_q },
		.title = AYU_T(ayu_RegexFiltersEnableSharedInChats),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filtersEnabledInChats()),
	});
	if (sharedButton) {
		sharedButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->filtersEnabledInChats());
		}) | on_next([=](bool enabled) {
			AyuSettings::getInstance().setFiltersEnabledInChats(enabled);
			FiltersCacheController::rebuildCache();
			FiltersCacheController::fireUpdate();
		}, sharedButton->lifetime());
	}

	const auto blockedButton = builder.addButton({
		.id = u"ayu/hideFromBlocked"_q,
		.title = AYU_T(ayu_FiltersHideFromBlocked),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->hideFromBlocked()),
	});
	if (blockedButton) {
		blockedButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->hideFromBlocked());
		}) | on_next([=](bool enabled) {
			AyuSettings::getInstance().setHideFromBlocked(enabled);
			FiltersCacheController::rebuildCache();
			FiltersCacheController::fireUpdate();
		}, blockedButton->lifetime());
	}

	builder.addSkip();
}

void BuildDeleteBypassKeywords(SectionBuilder &builder) {
	auto *settings = &AyuSettings::getInstance();

	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle(rpl::single(AyuHantHelper(
		qsl("ayu_DeleteBypassKeywordsTitle"),
		qsl("Delete by keywords"))));

	const auto enabledButton = builder.addButton({
		.id = u"ayu/deleteBypassKeywordsEnabled"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_DeleteBypassKeywordsEnable"),
			qsl("Enable direct deletion by keywords"))),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->deleteBypassKeywordsEnabled()),
	});
	if (enabledButton) {
		enabledButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->deleteBypassKeywordsEnabled());
		}) | on_next([=](bool enabled) {
			AyuSettings::getInstance().setDeleteBypassKeywordsEnabled(enabled);
		}, enabledButton->lifetime());
	}

	const auto regexModeButton = builder.addButton({
		.id = u"ayu/deleteBypassKeywordsRegexEnabled"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_DeleteBypassKeywordsRegexEnable"),
			qsl("Enable regex matching"))),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->deleteBypassKeywordsRegexEnabled()),
	});
	if (regexModeButton) {
		regexModeButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->deleteBypassKeywordsRegexEnabled());
		}) | on_next([=](bool enabled) {
			AyuSettings::getInstance().setDeleteBypassKeywordsRegexEnabled(enabled);
		}, regexModeButton->lifetime());
	}

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"ayu/deleteBypassKeywordsEdit"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_DeleteBypassKeywordsEdit"),
			qsl("Edit keyword list"))),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->show(Box([=](not_null<Ui::GenericBox*> box) {
					box->setTitle(rpl::single(AyuHantHelper(
						qsl("ayu_DeleteBypassKeywordsEdit"),
						qsl("Edit keyword list"))));

				auto initial = QString();
				for (const auto &keyword : AyuSettings::getInstance().deleteBypassKeywords()) {
					if (!initial.isEmpty()) {
						initial += u"\n"_q;
					}
					initial += keyword;
				}

				const auto input = box->addRow(
					object_ptr<Ui::InputField>(
						box->verticalLayout(),
						st::defaultInputField,
						Ui::InputField::Mode::MultiLine,
							rpl::single(AyuHantHelper(
								qsl("ayu_DeleteBypassKeywordsPlaceholder"),
								qsl("Split by new line, comma, or semicolon")))),
					st::settingsCheckboxPadding);
				input->setText(initial);

				const auto saveAndClose = [=] {
					auto normalized = input->getTextWithTags().text;
					normalized.replace(u"\r"_q, QString());

					std::vector<QString> keywords;
					const auto parts = normalized.split(
						QRegularExpression(u"[\\n,;\\x{FF0C}\\x{FF1B}]+"_q),
						Qt::SkipEmptyParts);
					for (const auto &row : parts) {
						const auto keyword = row.trimmed();
						if (!keyword.isEmpty()) {
							keywords.push_back(keyword);
						}
					}

					AyuSettings::getInstance().setDeleteBypassKeywords(keywords);
					box->closeBox();
				};

				input->submits() | rpl::on_next(saveAndClose, input->lifetime());
				box->addButton(tr::lng_settings_save(), saveAndClose);
				box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
				box->setFocusCallback([=] { input->setFocusFast(); });
			}));
		},
	});
	builder.addDividerText(rpl::single(AyuHantHelper(
		qsl("ayu_DeleteBypassKeywordsDescription"),
		qsl("Messages containing these keywords are deleted directly and not saved in anti-delete history."))));
	builder.addDividerText(rpl::single(AyuHantHelper(
		qsl("ayu_DeleteBypassKeywordsRegexDescription"),
		qsl("When regex matching is enabled, each line in the list is treated as a regular expression (invalid patterns are ignored)."))));

	builder.addSkip();
}

void BuildDeleteBypassUserIds(SectionBuilder &builder) {
	auto *settings = &AyuSettings::getInstance();
	const auto controller = builder.controller();

	const auto enabledButton = builder.addButton({
		.id = u"ayu/deleteBypassUserIdsEnabled"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_DeleteBypassUserIdsEnable"),
			qsl("Enable direct deletion by user IDs"))),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->deleteBypassUserIdsEnabled()),
	});
	if (enabledButton) {
		enabledButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->deleteBypassUserIdsEnabled());
		}) | on_next([=](bool enabled) {
			AyuSettings::getInstance().setDeleteBypassUserIdsEnabled(enabled);
		}, enabledButton->lifetime());
	}

	builder.addButton({
		.id = u"ayu/deleteBypassUserIdsEdit"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_DeleteBypassUserIdsEdit"),
			qsl("Edit user list"))),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->show(Box([=](not_null<Ui::GenericBox*> box) {
					box->setTitle(rpl::single(AyuHantHelper(
						qsl("ayu_DeleteBypassUserIdsEdit"),
						qsl("Edit user list"))));

				auto initial = QString();
				for (const auto userId : AyuSettings::getInstance().deleteBypassUserIds()) {
					if (!initial.isEmpty()) {
						initial += u"\n"_q;
					}
					initial += QString::number(userId);
				}

				const auto input = box->addRow(
					object_ptr<Ui::InputField>(
						box->verticalLayout(),
						st::defaultInputField,
						Ui::InputField::Mode::MultiLine,
							rpl::single(AyuHantHelper(
								qsl("ayu_DeleteBypassUserIdsPlaceholder"),
								qsl("Split by new line, comma, or semicolon (numbers only)")))),
					st::settingsCheckboxPadding);
				input->setText(initial);

				const auto saveAndClose = [=] {
					auto normalized = input->getTextWithTags().text;
					normalized.replace(u"\r"_q, QString());

					std::vector<long long> userIds;
					const auto parts = normalized.split(
						QRegularExpression(u"[\\n,;\\x{FF0C}\\x{FF1B}]+"_q),
						Qt::SkipEmptyParts);
					for (const auto &row : parts) {
						bool ok = false;
						const auto val = row.trimmed().toLongLong(&ok);
						if (ok) {
							userIds.push_back(val);
						}
					}

					AyuSettings::getInstance().setDeleteBypassUserIds(userIds);
					box->closeBox();
				};

				input->submits() | rpl::on_next(saveAndClose, input->lifetime());
				box->addButton(tr::lng_settings_save(), saveAndClose);
				box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
				box->setFocusCallback([=] { input->setFocusFast(); });
			}));
		},
	});
	builder.addDividerText(rpl::single(AyuHantHelper(
		qsl("ayu_DeleteBypassUserIdsDescription"),
		qsl("Messages from these users are deleted directly and not saved in anti-delete history."))));

	builder.addSkip();
}

void BuildShared(SectionBuilder &builder) {
	builder.addDivider();
	builder.addSkip();

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"ayu/sharedFilters"_q,
		.title = AYU_T(ayu_RegexFiltersShared),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->dialogId = std::nullopt;
			controller->showExclude = false;
			controller->showSettings(AyuFiltersList::Id());
		},
	});
}

void BuildShadowBan(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addButton({
		.id = u"ayu/shadowBanIds"_q,
		.altIds = { u"ayu/shadowBanList"_q },
		.title = AYU_T(ayu_FiltersShadowBan),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->dialogId = std::nullopt;
			controller->showExclude = false;
			controller->shadowBan = true;
			controller->showSettings(AyuFiltersList::Id());
		},
	});
}

void BuildPerDialog(SectionBuilder &builder) {
	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [&](const WidgetContext &wctx) {
			if (!AyuDatabase::hasPerDialogFilters()) {
				return;
			}

			const auto container = wctx.container;
			const auto controller = wctx.controller;

			AddSkip(container);
			AddDivider(container);

			auto ctrl = container->lifetime().make_state<PerDialogFiltersListController>(
				&controller->session(),
				controller);

			auto list = object_ptr<Ui::PaddingWrap<PeerListContent>>(
				container,
				object_ptr<PeerListContent>(
					container,
					ctrl),
				QMargins(0, -st::peerListBox.padding.top(), 0, -st::peerListBox.padding.bottom()));
			AddSkip(container);
			const auto content = container->add(std::move(list));
			AddSkip(container);
			auto delegate = container->lifetime().make_state<PeerListContentDelegateSimple>();
			delegate->setContent(content->entity());
			ctrl->setDelegate(delegate);
		}, [&](const SearchContext &) {
		});
	});
}

const auto kMeta = BuildHelper({
	.id = AyuFilters::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryFilters,
	.icon = &st::menuIconTagFilter,
}, [](SectionBuilder &builder) {
	BuildFiltersSettings(builder);
	BuildDeleteBypassKeywords(builder);
	BuildDeleteBypassUserIds(builder);
	BuildShared(builder);
	BuildShadowBan(builder);
	BuildPerDialog(builder);
});

} // namespace

rpl::producer<QString> AyuFilters::title() {
	return AYU_T(ayu_CategoryFilters);
}

void AyuFilters::fillTopBarMenu(const Ui::Menu::MenuCallback &addAction) {
	addAction(
		AYU_S(ayu_FiltersMenuSelectChat),
		[=] {
			if (const auto window = Core::App().activeWindow()) {
				if (const auto controller = window->sessionController()) {
					auto types = InlineBots::PeerTypes();
					types |= InlineBots::PeerType::Bot;
					types |= InlineBots::PeerType::Group;
					types |= InlineBots::PeerType::Broadcast;

					Window::ShowChooseRecipientBox(
						controller,
						[=](not_null<Data::Thread*> thread) {
							const auto peer = thread->peer();
							controller->dialogId = getDialogIdFromPeer(peer);
							controller->showExclude = true;
							controller->showSettings(AyuFiltersList::Id());
							return true;
						},
							AYU_T(ayu_FiltersMenuSelectChat),
						nullptr,
						types);
				}
			}
		},
		&st::menuIconSearch);
	addAction({ .isSeparator = true });
	addAction(
		AYU_S(ayu_FiltersMenuImport),
		[=] {
			auto box = Box(Ui::FillImportFiltersBox, true);
			Ui::show(std::move(box));
		},
		&st::menuIconArchive);
	if (AyuDatabase::hasFilters()) {
		addAction(
			AYU_S(ayu_FiltersMenuExport),
			[=] {
				auto box = Box(Ui::FillImportFiltersBox, false);
				Ui::show(std::move(box));
			},
			&st::menuIconUnarchive);
	}
	addAction({ .isSeparator = true });
	addAction(
		AYU_S(ayu_FiltersMenuClear),
		[=] {
			auto callback = [=](Fn<void()> &&close) {
				AyuDatabase::deleteAllFilters();
				AyuDatabase::deleteAllExclusions();
				FiltersCacheController::rebuildCache();
				FiltersCacheController::fireUpdate();
				close();
			};
			auto box = Ui::MakeConfirmBox({
				.text = AYU_S(ayu_FiltersClearPopupText),
				.confirmed = callback,
				.confirmText = AYU_S(ayu_FiltersClearPopupActionText)
			});
			Ui::show(std::move(box));
		},
		&st::menuIconClear);
}

AyuFilters::AyuFilters(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuFilters::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuFiltersId() {
	return AyuFilters::Id();
}

} // namespace Settings
