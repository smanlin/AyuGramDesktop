// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "shadow_ban_utils.h"

#include <QStringList>

#include "filters_cache_controller.h"
#include "ayu/ayu_settings.h"
#include "ayu/data/entities.h"

void ShadowBanUtils::addShadowBan(ID userId) {
	AyuSettings::getInstance().addShadowBan(userId);
}

void ShadowBanUtils::removeShadowBan(ID userId) {
	AyuSettings::getInstance().removeShadowBan(userId);
}

bool ShadowBanUtils::isShadowBanned(ID userId) {
	const auto &settings = AyuSettings::getInstance();
	return settings.isShadowBanned(userId);
}

void ShadowBanUtils::setShadowBanList() {
	FiltersCacheController::rebuildCache();
	FiltersCacheController::fireUpdate();
	AyuSettings::save();
}
