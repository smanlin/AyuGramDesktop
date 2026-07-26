// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_general.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/ayu_hant_helper.h"
#include "ayu/ui/settings/settings_ayu_utils.h"
#include "ayu/ui/settings/settings_main.h"
#include "core/application.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/boxes/confirm_box.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace AyBuilder;

namespace {

void BuildTranslator(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::lng_translate_settings_subtitle());

	auto *settings = &AyuSettings::getInstance();

	const auto options = std::vector{
		QString("Telegram"),
		QString("Google"),
		QString("Yandex")
	};

	const auto getIndex = [=](TranslationProvider val) {
		return static_cast<int>(val);
	};

	auto currentVal = AyuSettings::getInstance().translationProviderValue()
		| rpl::map(getIndex)
		| rpl::map([=](int val) { return options[val]; });

	const auto button = builder.addButton({
		.id = u"ayu/translationProvider"_q,
		.title = AYU_T(ayu_TranslationProvider),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			if (const auto controller = Core::App().activeWindow()->sessionController()) {
				controller->show(Box(
					[=](not_null<Ui::GenericBox*> box) {
						const auto save = [=](int index) {
							AyuSettings::getInstance().setTranslationProvider(
								static_cast<TranslationProvider>(index));
						};
						SingleChoiceBox(box, {
							.title = rpl::single(AYU_S(ayu_TranslationProvider)),
							.options = options,
							.initialSelection = getIndex(settings->translationProvider()),
							.callback = save,
						});
					}));
			}
		},
	});
	if (button) {
		ayu.addBetaBadge(button);
	}
}

void BuildShowPeerId(SectionBuilder &builder) {
	auto *settings = &AyuSettings::getInstance();

	const auto options = std::vector{
		AYU_S(ayu_SettingsShowID_Hide),
		QString("Telegram API"),
		QString("Bot API")
	};

	auto currentVal = AyuSettings::getInstance().showPeerIdValue()
		| rpl::map([=](PeerIdDisplay val) {
			return options[static_cast<int>(val)];
		});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"ayu/showPeerId"_q,
		.altIds = { u"ayu/showIdAndDc"_q },
		.title = AYU_T(ayu_SettingsShowID),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			controller->show(Box(
				[=](not_null<Ui::GenericBox*> box) {
					const auto save = [=](int index) {
						AyuSettings::getInstance().setShowPeerId(
							static_cast<PeerIdDisplay>(index));
					};
					SingleChoiceBox(box, {
							.title = rpl::single(AYU_S(ayu_SettingsShowID)),
						.options = options,
						.initialSelection = static_cast<int>(settings->showPeerId()),
						.callback = save,
					});
				}));
		},
	});
}

void BuildQoLToggles(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto *settings = &AyuSettings::getInstance();

	BuildTranslator(builder, ayu);
	ayu.addSectionDivider();

	builder.addSubsectionTitle(AYU_T(ayu_CategoryGeneral));

	ayu.addSettingToggle({
		.id = u"ayu/disableStories"_q,
		.altIds = { u"ayu/hideStories"_q },
		.title = AYU_T(ayu_DisableStories),
		.getter = &AyuSettings::disableStories,
		.setter = &AyuSettings::setDisableStories,
	});

	ayu.addSettingToggle({
		.id = u"ayu/disableOpenLinkWarning"_q,
		.title = AYU_T(ayu_DisableOpenLinkWarning),
		.getter = &AyuSettings::disableOpenLinkWarning,
		.setter = &AyuSettings::setDisableOpenLinkWarning,
	});

	ayu.addCollapsibleToggle({
		.id = u"ayu/similarChannels"_q,
		.title = AYU_T(ayu_DisableSimilarChannels),
		.checkboxes = {
			NestedEntry{
				AYU_S(ayu_CollapseSimilarChannels),
				[] { return AyuSettings::getInstance().collapseSimilarChannels(); },
				[](bool v) { AyuSettings::getInstance().setCollapseSimilarChannels(v); }
			},
			NestedEntry{
				AYU_S(ayu_HideSimilarChannelsTab),
				[] { return AyuSettings::getInstance().hideSimilarChannels(); },
				[](bool v) { AyuSettings::getInstance().setHideSimilarChannels(v); }
			}
		},
		.toggledWhenAll = true,
	});

	ayu.addSettingToggle({
		.id = u"ayu/disableNotificationsDelay"_q,
		.title = AYU_T(ayu_DisableNotificationsDelay),
		.getter = &AyuSettings::disableNotificationsDelay,
		.setter = &AyuSettings::setDisableNotificationsDelay,
	});

	ayu.addSectionDivider();

	const auto controller = builder.controller();
	const auto zalgoButton = builder.addButton({
		.id = u"ayu/filterZalgo"_q,
		.title = AYU_T(ayu_FilterZalgo),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filterZalgo()),
	});
	if (zalgoButton) {
		zalgoButton->toggledValue(
		) | rpl::filter(
			[=](bool enabled) {
				return (enabled != settings->filterZalgo());
			}
		) | on_next(
			[=](bool enabled) {
				AyuSettings::getInstance().setFilterZalgo(enabled);
				controller->show(Ui::MakeConfirmBox({
					.text = tr::lng_settings_need_restart(),
					.confirmed = [] { Core::Restart(); },
					.confirmText = tr::lng_settings_restart_now(),
					.cancelText = tr::lng_settings_restart_later(),
				}));
			},
			zalgoButton->lifetime());
		ayu.addBetaBadge(zalgoButton);
	}

	ayu.addSettingToggle({
		.id = u"ayu/improveLinkPreviews"_q,
		.title = AYU_T(ayu_ImproveLinkPreviews),
		.getter = &AyuSettings::improveLinkPreviews,
		.setter = &AyuSettings::setImproveLinkPreviews,
	});
	ayu.addSettingToggle({
		.id = u"ayu/showMessageSeconds"_q,
		.altIds = { u"ayu/formatTimeWithSeconds"_q },
		.title = AYU_T(ayu_SettingsShowMessageSeconds),
		.getter = &AyuSettings::showMessageSeconds,
		.setter = &AyuSettings::setShowMessageSeconds,
	});
	ayu.addSettingToggle({
		.id = u"ayu/showMessageId"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_SettingsShowMessageID"),
			qsl("Show message ID next to time"))),
		.getter = &AyuSettings::showMessageId,
		.setter = &AyuSettings::setShowMessageId,
	});
	ayu.addSettingToggle({
		.id = u"ayu/showViewJson"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_SettingsShowViewJson"),
			qsl("Show [View JSON] in context menu"))),
		.getter = &AyuSettings::showViewJson,
		.setter = &AyuSettings::setShowViewJson,
	});
	ayu.addSettingToggle({
		.id = u"ayu/alwaysShowSpoilerText"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_AlwaysShowSpoilerText"),
			qsl("Always show spoiler text"))),
		.getter = &AyuSettings::alwaysShowSpoilerText,
		.setter = &AyuSettings::setAlwaysShowSpoilerText,
	});
	ayu.addSettingToggle({
		.id = u"ayu/alwaysShowSpoilerMedia"_q,
		.title = rpl::single(AyuHantHelper(
			qsl("ayu_AlwaysShowSpoilerMedia"),
			qsl("Always show spoiler media"))),
		.getter = &AyuSettings::alwaysShowSpoilerMedia,
		.setter = &AyuSettings::setAlwaysShowSpoilerMedia,
	});

	BuildShowPeerId(builder);

	ayu.addSectionDivider();

	builder.addSubsectionTitle(
		rpl::single(AyuHantHelper(qsl("ayu_WebviewHeader"), QString("Webview"))));

	ayu.addSettingToggle({
		.id = u"ayu/spoofWebviewAsAndroid"_q,
		.title = AYU_T(ayu_SettingsSpoofWebviewAsAndroid),
		.getter = &AyuSettings::spoofWebviewAsAndroid,
		.setter = &AyuSettings::setSpoofWebviewAsAndroid,
	});

	ayu.addCollapsibleToggle({
		.id = u"ayu/biggerWindow"_q,
		.title = AYU_T(ayu_SettingsBiggerWindow),
		.checkboxes = {
			NestedEntry{
				AYU_S(ayu_SettingsIncreaseWebviewHeight),
				[] { return AyuSettings::getInstance().increaseWebviewHeight(); },
				[](bool v) { AyuSettings::getInstance().setIncreaseWebviewHeight(v); }
			},
			NestedEntry{
				AYU_S(ayu_SettingsIncreaseWebviewWidth),
				[] { return AyuSettings::getInstance().increaseWebviewWidth(); },
				[](bool v) { AyuSettings::getInstance().setIncreaseWebviewWidth(v); }
			}
		},
		.toggledWhenAll = false,
	});

	ayu.addSectionDivider();

	builder.addSubsectionTitle(AYU_T(ayu_ConfirmationsTitle));

	ayu.addSettingToggle({
		.id = u"ayu/stickerConfirmation"_q,
		.title = AYU_T(ayu_StickerConfirmation),
		.getter = &AyuSettings::stickerConfirmation,
		.setter = &AyuSettings::setStickerConfirmation,
	});
	ayu.addSettingToggle({
		.id = u"ayu/gifConfirmation"_q,
		.title = AYU_T(ayu_GIFConfirmation),
		.getter = &AyuSettings::gifConfirmation,
		.setter = &AyuSettings::setGifConfirmation,
	});
	ayu.addSettingToggle({
		.id = u"ayu/voiceConfirmation"_q,
		.title = AYU_T(ayu_VoiceConfirmation),
		.getter = &AyuSettings::voiceConfirmation,
		.setter = &AyuSettings::setVoiceConfirmation,
	});
}

const auto kMeta = BuildHelper({
	.id = AyuGeneral::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryGeneral,
	.icon = &st::menuIconShowAll,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	builder.addSkip();
	BuildQoLToggles(builder, ayu);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> AyuGeneral::title() {
	return tr::ayu_CategoryGeneral();
}

AyuGeneral::AyuGeneral(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuGeneralId() {
	return AyuGeneral::Id();
}

} // namespace Settings
