#include <Geode/modify/LevelListLayer.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

#define TELEPORT_AMOUNT 10.f

enum class Action {
	Disabled = 0,
	AutoKill = 1,
	TelPAway = 2,
	PushButn = 3
};

// banning the number itself, and "common" "misspellings"
// then banning roman numerals, binary, and hexadecmial
// then banning reversed variants of all the above
// this is more effecient than plain regex
// i would ban pinyin too but that's way too far
static const std::vector<std::string> blacklist = {
	"67",
	"6ix", "7even", "six", "seven", "sevn",
	"xi6", "neve7", "xis", "neves", "nves",
	"lxvii", "vivii", "vii", "lx", "vi",
	"iivxl", "iiviv", "iiv", "xl", "iv",
	"01000011", "0x43",
	"11000010", "34x0",
	"6", "7"
};

bool noObjectIDs = true;
bool noGroupIDs = true;
bool noEditorLayers = true;
bool noColorChannels = true;

bool noHarderLevels = true;
bool noOffendingIcons = true;
bool noHarderLevelsInsideLevelLists = true;

bool noOffendingLevelIDsConsecutively = true;
bool noOffendingLevelIDsIndividually = true;
bool noOffendingLevelIDsExclusiveOr = true;

bool noLevelsContainingName = true;

bool noOffendingLevelListIDsConsecutively = true;
bool noOffendingLevelListIDsIndividually = true;
bool noOffendingLevelListIDsExclusiveOr = true;

bool noLevelListsContainingName = true;

bool noLevelListsWithOffendingRewardCounts = true;
bool noLevelListsWithOffendingLevelClaimRequirements = true;

bool noOffendingLevelsWithOffendingTextObjects = true;

bool closeIconKitForStats = true;

Action handleXPosition = Action::AutoKill;
Action handleYPosition = Action::AutoKill;

bool autoKillOnPercentageClassic = true;
bool autoKillOnSpeedrunPlatformr = true;

bool replaceAllText = false;

bool closeProfilesForGlobalRank = false;
bool closeProfilesForAnyStats = false;
bool closeProfilesForUsername = false;
bool closeProfilesForUserID = false;
bool closeProfilesForID = false;

bool enabled = true;

bool gameHasStarted = false;

bool calledAlready = false;
bool nextKeyWhen = false;
bool crowdedGarageStats = false;
bool separateDualIcons = false;

// this is my own mod. leave me alone.
bool unpauseOnUnfocus = false;
bool unpauseOnUnfocusOriginalSofttoggle = false;

class GaragePopup final : public geode::Popup<> {}; // dummy declaration to fake escape key call on parent for YetAnotherModMenu

void assignAppropriately(const std::string& settingValue, Action& settingBeingAssigned) {
	if (settingValue == "auto-kill") settingBeingAssigned = Action::AutoKill;
	else if (settingValue == "press jump") settingBeingAssigned = Action::PushButn;
	else if (settingValue == "teleport away") settingBeingAssigned = Action::TelPAway;
	else settingBeingAssigned = Action::Disabled;
}

bool isEitherSixOrSeven(const int& value) {
	return value == 6 || value == 7;
}

bool valueHasTheTwoDigits(const int& value) {
	if (value < 67) return false;
	if (value < 68) return true;

	int valueAsInt = value;
	int previousDigit = valueAsInt % 10;
	valueAsInt /= 10;

	while (valueAsInt != 0) {
		const int currentDigit = valueAsInt % 10;
		if (currentDigit == 6 && previousDigit == 7) return true;
		previousDigit = currentDigit;
		valueAsInt /= 10;
	}

	return false;
}

bool containsBothDigitsConsecutively(const std::string& stringifiedID) {
	return geode::utils::string::contains(stringifiedID, "67");
}
bool containsBothDigitsIndividually(const std::string& stringifiedID) {
	return !containsBothDigitsConsecutively(stringifiedID) && geode::utils::string::contains(stringifiedID, '6') && geode::utils::string::contains(stringifiedID, '7');
}
bool containsEitherDigitsExclusiveOr(const std::string& stringifiedID) {
	return !containsBothDigitsConsecutively(stringifiedID) && !containsBothDigitsIndividually(stringifiedID) && (geode::utils::string::contains(stringifiedID, '6') ^ geode::utils::string::contains(stringifiedID, '7'));
}

static const std::pair<std::string, bool*> booleanSettings[] = {
	{"enabled", &enabled},
	{"replaceAllText", &replaceAllText},
	{"autoKillOnTime", &autoKillOnSpeedrunPlatformr},
	{"autoKillOnPercent", &autoKillOnPercentageClassic},
	{"closeProfilesForID", &closeProfilesForID},
	{"closeProfilesForUserID", &closeProfilesForUserID},
	{"closeProfilesForGlobalRank", &closeProfilesForGlobalRank},
	{"closeProfilesForAnyStats", &closeProfilesForAnyStats},
	{"closeProfilesForUsername", &closeProfilesForUsername},
	{"noObjectIDs", &noObjectIDs},
	{"noGroupIDs", &noGroupIDs},
	{"noEditorLayers", &noEditorLayers},
	{"noColorChannels", &noColorChannels},
	{"noHarderLevels", &noHarderLevels},
	{"noHarderLevelsInsideLevelLists", &noHarderLevelsInsideLevelLists},
	{"noOffendingLevelIDsConsecutively", &noOffendingLevelIDsConsecutively},
	{"noOffendingLevelIDsIndividually", &noOffendingLevelIDsIndividually},
	{"noOffendingLevelIDsExclusiveOr", &noOffendingLevelIDsExclusiveOr},
	{"noLevelsContainingName", &noLevelsContainingName},
	{"noLevelListsContainingName", &noLevelListsContainingName},
	{"noOffendingLevelIDsIndividually", &noOffendingLevelIDsIndividually},
	{"noOffendingLevelIDsExclusiveOr", &noOffendingLevelIDsExclusiveOr},
	{"noOffendingLevelListIDsConsecutively", &noOffendingLevelListIDsConsecutively},
	{"noOffendingLevelListIDsIndividually", &noOffendingLevelListIDsIndividually},
	{"noLevelListsWithOffendingRewardCounts", &noLevelListsWithOffendingRewardCounts},
	{"noLevelListsWithOffendingLevelClaimRequirements", &noLevelListsWithOffendingLevelClaimRequirements},
	{"closeIconKitForStats", &closeIconKitForStats},
	{"noOffendingIcons", &noOffendingIcons},
	{"noOffendingLevelsWithOffendingTextObjects", &noOffendingLevelsWithOffendingTextObjects},
};

template<typename Function>
void repeatedlyQueueFunction(int times, Function&& function){
	if (times < 1) return function();
	Loader::get()->queueInMainThread([times, fn = std::forward<Function>(function)]() mutable {
		if (times == 1) fn();
		else repeatedlyQueueFunction(times - 1, std::move(fn));
	});
}

std::string removeColorTags(const std::string& originalString) {
	return geode::utils::string::replace(geode::utils::string::replace(geode::utils::string::replace(geode::utils::string::replace(originalString, "<cj>", ""), "<cl>", ""), "<c_>", ""), "</c>", "");
}

static constexpr std::pair<std::string, const char*> gameStatsManagerStats[] = {
	// vanilla stats
	{"<cy>stars</c>", "6"},
	{"<co>secret coins</c>", "8"},
	{"<cc>user coins</c>", "12"},
	{"<cl>diamonds</c>", "13"},
	{"<cj>orbs</c>", "14"},
	{"<cl>moons</c>", "28"},
	{"<cl>diamond shards</c>", "29"},
	// next key when? OR crowded garage stats
	{"<cl>demon keys</c>", "21"},
	// crowded garage stats
	{"<cr>demons</c>", "5"},
	{"<co>gold keys</c>", "43"},
	// NOTE: bonus shards not included, since those values depend on the lowest of each tier's previous shards anyway, so that would be redundant
	// tier I shards
	{"shadow shards", "16"},
	{"poison shards", "17"},
	{"fire shards", "18"},
	{"ice shards", "19"},
	{"lava shards", "20"},
	// tier II shards
	{"earth shards", "23"},
	{"blood shards", "24"},
	{"metal shards", "25"},
	{"light shards", "26"},
	{"soul shards", "27"}
};

std::pair<std::string, int> checkForOffendingGSMStats() {
	std::string offendingStat = "unknown";
	int offendingStatValue = -1;
	GameStatsManager* gsm = GameStatsManager::get();
	for (auto const& [label, gameManagerKey] : gameStatsManagerStats) {
		// the idea is to read the gamemanager variable the same way these mods do it.
		// if the mod the state depends on isnt installed, why bother reading it?
		// hence, the early continue statements
		if ((label == "<co>gold keys</c>" || label == "<cr>demons</c>") && !crowdedGarageStats) continue;
		if (label == "<cl>demon keys</c>" && !nextKeyWhen && !crowdedGarageStats) continue;
		const int value = gsm->getStat(gameManagerKey);
		if (isEitherSixOrSeven(value) || valueHasTheTwoDigits(value)) {
			offendingStat = label;
			offendingStatValue = value;
			break;
		}
	}
	return {offendingStat, offendingStatValue};
}

static std::pair<std::string, int GJUserScore::*> gjUserScoreStats[] = {
	{"<cy>stars</c>", &GJUserScore::m_stars},
	{"<cl>moons</c>", &GJUserScore::m_moons},
	{"<co>secret coins</c>", &GJUserScore::m_secretCoins},
	{"<cc>user coins</c>", &GJUserScore::m_userCoins},
	{"<cr>demons</c>", &GJUserScore::m_demons},
	{"creator points", &GJUserScore::m_creatorPoints},
	{"<cl>diamonds</c>", &GJUserScore::m_diamonds},
};

std::pair<std::string, int> checkForOffendingGJUSStats(const GJUserScore* userInfo) {
	std::string offendingStat = "unknown";
	int offendingStatValue = -1;
	for (auto const& [label, member] : gjUserScoreStats) {
		const int value = userInfo->*member;
		if (valueHasTheTwoDigits(value)) {
			offendingStat = label;
			offendingStatValue = value;
			break;
		}
	}
	return {offendingStat, offendingStatValue};
}

static std::pair<std::string, geode::SeedValueRSV GameManager::*> gameManagerIcons[] = {
	{"<cg>cube</c>", &GameManager::m_playerFrame},
	{"<cp>ship</c>", &GameManager::m_playerShip},
	{"<cr>ball</c>", &GameManager::m_playerBall},
	{"<co>UFO</c>", &GameManager::m_playerBird},
	{"<cj>wave</c>", &GameManager::m_playerDart},
	{"robot", &GameManager::m_playerRobot},
	{"<ca>spider</c>", &GameManager::m_playerSpider},
	{"<co>primary</c> <co>color</c>", &GameManager::m_playerColor},
	{"<cj>secondary</c> <cj>color</c>", &GameManager::m_playerColor2},
	{"<co>glow</c> <co>color</c>", &GameManager::m_playerGlowColor},
	// there aren't even close to 67 of these guys yet so ignore them
	// {"<cy>swing</c>", &GameManager::m_playerSwing},
	// {"<cp>jetpack</c>", &GameManager::m_playerJetpack},
	// {"<cr>p</c><co>l</c><cs>a</c><cc>y</c><cy>e</c><cg>r</c> <cj>t</c><cl>r</c><cb>a</c><ca>i</c><cr>l</c>", &GameManager::m_playerStreak},
	// {"<cr>s</c><co>h</c><cs>i</c><cc>p</c> <cy>t</c><cg>r</c><cj>a</c><cl>i</c><cb>l</c> <ca>fi</c><cr>re</c>", &GameManager::m_playerShipFire},
	// {"<cr>d</c><co>e</c><cs>a</c><cc>t</c><cy>h</c> <cg>e</c><cj>f</c><cl>f</c><cb>e</c><ca>c</c><cr>t</c>", &GameManager::m_playerDeathEffect},
};

std::pair<std::string, int> checkForOffendingGMIcons() {
	std::string offendingMode = "unknown";
	int offendingIconValue = -1;
	for (auto const& [label, member] : gameManagerIcons) {
		geode::SeedValueRSV value = GameManager::get()->*member;
		if (valueHasTheTwoDigits(value.value())) {
			offendingMode = label;
			offendingIconValue = value;
			break;
		}
	}
	return {offendingMode, offendingIconValue};
}

static std::pair<std::string, std::string> weebifyingSeparateDualIcons[] = {
	{"<cg>cube</c>", "cube"},
	{"<cp>ship</c>", "ship"},
	{"<cr>ball</c>", "roll"},
	{"<co>UFO</c>", "bird"},
	{"<cj>wave</c>", "dart"},
	{"robot", "robot"},
	{"<ca>spider</c>", "spider"},
	{"<co>primary</c> <co>color</c>", "color1"},
	{"<cj>secondary</c> <cj>color</c>", "color2"},
	{"<co>glow</c> <co>color</c>", "colorglow"},
	// weebify doesn't std::clamp these guys yet so let's not ignore them
	{"<cy>swing</c>", "swing"},
	{"<cp>jetpack</c>", "jetpack"},
	{"<cr>p</c><co>l</c><cs>a</c><cc>y</c><cy>e</c><cg>r</c> <cj>t</c><cl>r</c><cb>a</c><ca>i</c><cr>l</c>", "trail"},
	{"<cr>s</c><co>h</c><cs>i</c><cc>p</c> <cy>t</c><cg>r</c><cj>a</c><cl>i</c><cb>l</c> <ca>fi</c><cr>re</c>", "shiptrail"},
	{"<cr>d</c><co>e</c><cs>a</c><cc>t</c><cy>h</c> <cg>e</c><cj>f</c><cl>f</c><cb>e</c><ca>c</c><cr>t</c>", "death"},
};

std::pair<std::string, int> checkForOffendingSDIIcons() {
	std::string offendingMode = "unknown";
	int offendingIconValue = -1;
	if (!separateDualIcons) return {offendingMode, offendingIconValue};
	Mod* sdi = Loader::get()->getLoadedMod("weebify.separate_dual_icons");
	for (auto const& [label, member] : weebifyingSeparateDualIcons) {
		int value = sdi->getSavedValue<int>(member, 1);
		if (valueHasTheTwoDigits(value)) {
			offendingMode = label;
			offendingIconValue = value;
			break;
		}
	}
	return {offendingMode, offendingIconValue};
}

$on_mod(Loaded) {
	for (auto &[settingName, booleanPointer] : booleanSettings) {
		*booleanPointer = Mod::get()->getSettingValue<bool>(settingName);
		listenForSettingChanges(settingName, [ptr = booleanPointer](const bool v){ *ptr = v; });
	}

	assignAppropriately(geode::utils::string::toLower(Mod::get()->getSettingValue<std::string>("handleYPositionBy")), handleYPosition);
	listenForSettingChanges("handleYPositionBy", [](const std::string& handleYPositionByNew) {
		assignAppropriately(geode::utils::string::toLower(handleYPositionByNew), handleYPosition);
	});

	assignAppropriately(geode::utils::string::toLower(Mod::get()->getSettingValue<std::string>("handleXPositionBy")), handleXPosition);
	listenForSettingChanges("handleXPositionBy", [](const std::string& handleXPositionByNew) {
		assignAppropriately(geode::utils::string::toLower(handleXPositionByNew), handleXPosition);
	});
}

class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;

		if (calledAlready) return true;
		calledAlready = true;

		nextKeyWhen = Loader::get()->isModLoaded("alphalaneous.next_key_when");
		crowdedGarageStats = Loader::get()->isModLoaded("omgrod.bettergaragestats"); // lord forgive me for this crime i have made
		separateDualIcons = Loader::get()->isModLoaded("weebify.separate_dual_icons");
		unpauseOnUnfocus = Loader::get()->isModLoaded("raydeeux.dontpauseonunfocus");

		return true;
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	struct Fields {
		bool hasGroupIDForbidden = false;
		bool hasObjectIDForbidden = false;
		bool hasEditorLayerForbidden = false;
		bool hasColorChannelForbidden = false;
		int forbiddenObjectID = -1;
		int forbiddenGroupID = -1;
		int forbiddenEditorLayer = -1;
		int forbiddenColorChannel = -1;
		std::string reason;
		bool hasForbiddenTextObject = false;
		std::string forbiddenTextObjectContents;
	};
	void kickFromPlayLayerGracefully(float dt) {
		if (!this->m_uiLayer || !this->m_uiLayer->m_pauseBtn) return;
		if (unpauseOnUnfocus) {
			Mod* upOUf = Loader::get()->getLoadedMod("raydeeux.dontpauseonunfocus");
			unpauseOnUnfocusOriginalSofttoggle = upOUf->getSettingValue<bool>("enabled");
			upOUf->setSettingValue<bool>("enabled", false);
		}
		this->m_uiLayer->m_pauseBtn->activate();
		FLAlertLayer* alert = geode::createQuickPopup("Anti5ix5even", fmt::format("You are unable to play <cy>{}</c>:\n<c_>{}</c>", static_cast<std::string>(this->m_level->m_levelName), m_fields->reason), "I Understand", nullptr, [this](FLAlertLayer* alert, bool close) {
			if (PlayLayer::get()) {
				PlayLayer::removeAllObjects();
				PlayLayer::onQuit();
			}
			FMODAudioEngine::get()->stopAllMusic(true);
			FMODAudioEngine::get()->stopAllEffects();
			GameManager::get()->fadeInMenuMusic();
			if (unpauseOnUnfocus) {
				Mod* upOUf = Loader::get()->getLoadedMod("raydeeux.dontpauseonunfocus");
				upOUf->setSettingValue<bool>("enabled", unpauseOnUnfocusOriginalSofttoggle);
			}
		});
		CCScene::get()->addChild(alert);
		alert->show();
	}
	bool leaveFor(const std::string& reason) {
		if (!enabled || !this->m_level) return false;
		m_fields->reason = reason;
		this->scheduleOnce(schedule_selector(MyPlayLayer::kickFromPlayLayerGracefully), .5f);
		return true;
	}
	static void updateLabel(const PlayLayer* pl, const std::string& reason) {
		CCNode* notifLabel = pl->m_uiLayer->getChildByID("anti5ix5even-notif-label"_spr);
		if (!notifLabel) return;

		notifLabel->stopAllActions();
		notifLabel->setPositionY((pl->m_uiLayer->getContentSize().height / 4.f) + 25.f);

		static_cast<CCLabelBMFont*>(notifLabel)->setOpacity(255);
		static_cast<CCLabelBMFont*>(notifLabel)->setString(removeColorTags(reason).c_str());

		notifLabel->runAction(CCSequence::create(CCDelayTime::create(.25f), CCSpawn::create(CCFadeOut::create(.5f), CCMoveTo::create(.5f, {pl->m_uiLayer->getContentSize().width / 2.f, pl->m_uiLayer->getContentSize().height / 4.f}), nullptr), nullptr));
	}
	static void teleportPlayerAwayX(const PlayerObject* player, float& value, const PlayLayer* pl) {
		const bool isPlayerOne = pl->m_player1 == player;
		MyPlayLayer::updateLabel(pl, fmt::format("Teleported Player {} away from X position {}!", isPlayerOne ? "1" : "2", static_cast<int>(value)));
		float incrementValue = TELEPORT_AMOUNT;
		if (player->m_isSideways) {
			if (player->m_isUpsideDown) incrementValue = incrementValue * -1.f;
			else incrementValue *= 1.f;
			if (player->m_isShip || player->m_isBird || player->m_isDart || player->m_isSwing) {
				incrementValue *= -1.f;
				if (player->m_holdingButtons.at(static_cast<int>(PlayerButton::Jump)) && (player->m_isShip || player->m_isDart)) {
					incrementValue *= -1.f;
				}
			}
		} else if (player->m_isGoingLeft) incrementValue *= -1.f;
		else incrementValue = incrementValue * 1.f;
		value += incrementValue;
	}
	static void teleportPlayerAwayY(const PlayerObject* player, float& value, const PlayLayer* pl) {
		const bool isPlayerOne = pl->m_player1 == player;
		MyPlayLayer::updateLabel(pl, fmt::format("Teleported Player {} away from Y position {}!", isPlayerOne ? "1" : "2", static_cast<int>(value)));
		float incrementValue = TELEPORT_AMOUNT;
		if (player->m_isSideways) {
			if (!player->m_isGoingLeft) incrementValue = incrementValue * 1.f;
			else incrementValue *= -1.f;
		} else if (player->m_isUpsideDown) incrementValue *= -1.f;
		else incrementValue = incrementValue * 1.f;
		if (player->m_isShip || player->m_isBird || player->m_isDart || player->m_isSwing) {
			incrementValue *= -1.f;
			if (player->m_holdingButtons.at(static_cast<int>(PlayerButton::Jump)) && (player->m_isShip || player->m_isDart)) {
				incrementValue *= -1;
			}
		}
		value += incrementValue;
	}
	void checkPlayerPosFor(PlayerObject* player) {
		if (!player || !enabled) return;
		const int xRoundedPosition = static_cast<int>(player->m_position.x);
		const std::string& oneOrTwo = player == this->m_player1 ? "1" : "2";
		if (handleXPosition != Action::Disabled && valueHasTheTwoDigits(xRoundedPosition)) {
			if (handleXPosition == Action::AutoKill) {
				MyPlayLayer::updateLabel(this, fmt::format("Auto-killed Player {} at X position {}!", oneOrTwo, xRoundedPosition));
				return PlayLayer::destroyPlayer(player, player);
			}
			if (handleXPosition == Action::PushButn || (player->m_isSpider && player->m_isSideways)) {
				MyPlayLayer::updateLabel(this, fmt::format("Player {} automatically jumped at X position {}!", oneOrTwo, xRoundedPosition));
				PlayLayer::queueButton(static_cast<int>(PlayerButton::Jump), true, player == this->m_player2);
				return PlayLayer::queueButton(static_cast<int>(PlayerButton::Jump), false, player == this->m_player2);
			}
			if (handleXPosition == Action::TelPAway) {
				return MyPlayLayer::teleportPlayerAwayX(player, player->m_position.x, this);
			}
		}
		const int yRoundedPosition = static_cast<int>(player->m_position.y);
		if (handleYPosition != Action::Disabled && valueHasTheTwoDigits(yRoundedPosition)) {
			if (handleYPosition == Action::AutoKill) {
				MyPlayLayer::updateLabel(this, fmt::format("Auto-killed Player {} at Y position {}!", oneOrTwo, yRoundedPosition));
				return PlayLayer::destroyPlayer(player, player);
			}
			if (handleYPosition == Action::PushButn || (player->m_isSpider && !player->m_isSideways)) {
				MyPlayLayer::updateLabel(this, fmt::format("Player {} automatically jumped at Y position {}!", oneOrTwo, yRoundedPosition));
				PlayLayer::queueButton(static_cast<int>(PlayerButton::Jump), true, player == this->m_player2);
				return PlayLayer::queueButton(static_cast<int>(PlayerButton::Jump), false, player == this->m_player2);
			}
			if (handleYPosition == Action::TelPAway) {
				return MyPlayLayer::teleportPlayerAwayY(player, player->m_position.y, this);
			}
		}
	}
	bool checkForBlacklistedNumber() {
		if (!this->m_level || !enabled || !this->m_uiLayer) return false;

		if (noOffendingIcons) {
			const auto [gamemode, iconID] = checkForOffendingGMIcons();
			if (gamemode != "unknown" && iconID > 0) return MyPlayLayer::leaveFor(fmt::format("You are currently using\na forbidden icon ID:</c> {} <cl>#</c><cl>{}</c><c_>.</c><c_>", gamemode, iconID));
		}

		if (noOffendingIcons && separateDualIcons) {
			const auto [gamemode, iconID] = checkForOffendingSDIIcons();
			if (gamemode != "unknown" && iconID > 0) return MyPlayLayer::leaveFor(fmt::format("You are currently using</c>\n<c_>a forbidden icon ID for player 2:</c> {} <cl>#</c><cl>{}</c><c_>.</c><c_>", gamemode, iconID));
		}

		if (const auto fields = m_fields.self()) {
			if (noOffendingLevelsWithOffendingTextObjects && fields->hasForbiddenTextObject) return MyPlayLayer::leaveFor(fmt::format("Level contains a text object that contains</c>\n<c_>forbidden content:</c> <cl>{}</c><c_>.</c><c_>", fields->forbiddenTextObjectContents));
			if (noObjectIDs && fields->hasObjectIDForbidden) return MyPlayLayer::leaveFor(fmt::format("Level contains an object that uses</c>\n<c_>a forbidden</c> <cj>object ID</c><c_>:</c> <cl>{}</c><c_>.</c><c_>", fields->forbiddenObjectID));
			if (noGroupIDs && fields->hasGroupIDForbidden) return MyPlayLayer::leaveFor(fmt::format("Level contains an object that uses</c>\n<c_>a forbidden</c> <cj>group ID</c><c_>:</c> <cl>{}</c><c_>.</c><c_>", fields->forbiddenGroupID));
			if (noEditorLayers && fields->hasEditorLayerForbidden) return MyPlayLayer::leaveFor(fmt::format("Level contains an object that uses</c>\n<c_>a forbidden</c> <cj>editor layer ID</c><c_>:</c> <cl>{}</c><c_>.</c><c_>", fields->forbiddenEditorLayer));
			if (noColorChannels && fields->hasColorChannelForbidden) return MyPlayLayer::leaveFor(fmt::format("Level contains an object that uses</c>\n<c_>a forbidden</c> <cj>color channel ID</c><c_>:</c> <cl>{}</c><c_>.</c><c_>", fields->forbiddenColorChannel));
		}

		if (noHarderLevels && isEitherSixOrSeven(this->m_level->m_stars.value())) {
			return MyPlayLayer::leaveFor(fmt::format("Level's</c> <cj>star/moon count</c> <c_>is forbidden:</c> <cl>{}</c><c_>.</c><c_>", this->m_level->m_stars.value()));
		}

		const std::string& levelIDAsString = fmt::to_string(this->m_level->m_levelID.value());
		if (noOffendingLevelIDsConsecutively && containsBothDigitsConsecutively(levelIDAsString)) {
			return MyPlayLayer::leaveFor(fmt::format("</c><cj>Level ID</c> <c_>contains</c>\n<c_>\"The Two Digits\" consecutively:</c> <cl>{}</c><c_>.</c><c_>", levelIDAsString));
		}
		if (noOffendingLevelIDsIndividually && containsBothDigitsIndividually(levelIDAsString)) {
			return MyPlayLayer::leaveFor(fmt::format("</c><cj>Level ID</c> <c_>contains both of</c>\n<c_>\"The Two Digits\" in individual spots:</c> <cl>{}</c><c_>.</c><c_>", levelIDAsString));
		}
		if (noOffendingLevelIDsExclusiveOr && containsEitherDigitsExclusiveOr(levelIDAsString)) {
			return MyPlayLayer::leaveFor(fmt::format("</c><cj>Level ID</c> <c_>contains either one of</c>\n<c_>\"The Two Digits\" at least once:</c> <cl>{}</c><c_>.</c><c_>", levelIDAsString));
		}

		if (noLevelsContainingName && geode::utils::string::containsAny(geode::utils::string::toLower(static_cast<std::string>(m_level->m_levelName)), blacklist)) {
			return MyPlayLayer::leaveFor("Level</c> <cj>name</c> <c_>is forbidden.");
		}

		if (CCNode* notifLabel = this->m_uiLayer->getChildByID("anti5ix5even-notif-label"_spr)) {
			static_cast<CCLabelBMFont*>(notifLabel)->setString("All good! Starting level now.");
			notifLabel->runAction(CCSequence::create(CCDelayTime::create(.25f), CCSpawn::create(CCFadeOut::create(.5f), CCMoveTo::create(.5f, {this->m_uiLayer->getContentSize().width / 2.f, this->m_uiLayer->getContentSize().height / 4.f}), nullptr), nullptr));
		}

		gameHasStarted = true;
		return false;
	}
	void addObject(GameObject* object) {
		PlayLayer::addObject(object);

		auto* fields = m_fields.self();
		if (!fields || !object || !enabled) return;

		if (!fields->hasForbiddenTextObject && noOffendingLevelsWithOffendingTextObjects && object->m_objectID == 914) {
			if (geode::utils::string::containsAny(geode::utils::string::toLower(static_cast<std::string>(static_cast<TextGameObject*>(object)->m_text)), blacklist)) {
				fields->hasForbiddenTextObject = true;
				fields->forbiddenTextObjectContents = static_cast<TextGameObject*>(object)->m_text;
			}
		}

		if (!fields->hasObjectIDForbidden && valueHasTheTwoDigits(object->m_objectID)) {
			fields->hasObjectIDForbidden = true;
			fields->forbiddenObjectID = object->m_objectID;
		}
		if (!fields->hasEditorLayerForbidden && (valueHasTheTwoDigits(object->m_editorLayer) || valueHasTheTwoDigits(object->m_editorLayer2))) {
			fields->hasEditorLayerForbidden = true;
			fields->forbiddenEditorLayer = valueHasTheTwoDigits(object->m_editorLayer) ? object->m_editorLayer : object->m_editorLayer2;
		}
		if (!fields->hasColorChannelForbidden && (valueHasTheTwoDigits(object->m_activeMainColorID) || valueHasTheTwoDigits(object->m_activeDetailColorID))) {
			fields->hasColorChannelForbidden = true;
			fields->forbiddenColorChannel = valueHasTheTwoDigits(object->m_activeMainColorID) ? object->m_activeMainColorID : object->m_activeDetailColorID;
		}

		if (!fields->hasGroupIDForbidden) return;
		for (int group : *object->m_groups) {
			if (!valueHasTheTwoDigits(group)) continue;
			fields->hasGroupIDForbidden = true;
			fields->forbiddenGroupID = group;
			break;
		}
	}
	void startGame() {
		gameHasStarted = false;
		PlayLayer::startGame();
		if (!enabled || !this->m_uiLayer || !this->m_uiLayer->m_pauseBtn || !this->m_level) return;
		(void) MyPlayLayer::checkForBlacklistedNumber();
	}
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
		gameHasStarted = false;
		if (!this->m_level || !enabled || !this->m_uiLayer) return;

		CCLabelBMFont* notifLabel = CCLabelBMFont::create("Checking for \"The Two Digits\"...", "chatFont.fnt");
		notifLabel->setBlendFunc({GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA});
		this->m_uiLayer->addChild(notifLabel);
		notifLabel->setPosition(this-> m_uiLayer->getContentSize() / 2.f);
		notifLabel->setID("anti5ix5even-notif-label"_spr);
		notifLabel->setTag(10212025);
	}
	void postUpdate(float p0) {
		PlayLayer::postUpdate(p0);
		if (!this->m_player1 || !this->m_player2 || !enabled || !gameHasStarted) return;

		MyPlayLayer::checkPlayerPosFor(this->m_player1);
		if (this->m_gameState.m_isDualMode && this->m_player2) MyPlayLayer::checkPlayerPosFor(this->m_player2);
	}
	void updateInfoLabel() {
		if (!this->m_level || this->m_level->isPlatformer() || !this->m_player1 || !enabled || !gameHasStarted || !autoKillOnPercentageClassic) return PlayLayer::updateInfoLabel();

		const int currPerc = PlayLayer::getCurrentPercentInt();
		log::info("currPerc: {}", currPerc);
		if (valueHasTheTwoDigits(currPerc)) {
			PlayLayer::destroyPlayer(this->m_player1, this->m_player1);
			MyPlayLayer::updateLabel(this, fmt::format("Killed Player 1 as they reached {}%!", currPerc));
		}

		PlayLayer::updateInfoLabel();
	}
	// why the hell does robtop not store the parameters in gamestate this is bullshit
	void updateTimeLabel(int seconds, int milliseconds, bool iDontFuckingKnow) {
		if (!this->m_level || !this->m_level->isPlatformer() || !this->m_player1 || !enabled || !gameHasStarted || !autoKillOnSpeedrunPlatformr) return PlayLayer::updateTimeLabel(seconds, milliseconds, iDontFuckingKnow);

		if (valueHasTheTwoDigits(seconds)) {
			PlayLayer::destroyPlayer(this->m_player1, this->m_player1);
			MyPlayLayer::updateLabel(this, fmt::format("Killed Player 1 as the timer reached {} seconds!", seconds));
		}

		PlayLayer::updateTimeLabel(seconds, milliseconds, iDontFuckingKnow);
	}
};

// thank you prevter for the code (i was gonna iterate manually and replace but this works too)
// geode::utils::string::toLower(str) is needed for case-insensitive matches.
// if that means copying extra strings so i can preserve original string's capitalization, so be it.
class $modify(MyCCLabelBMFont, CCLabelBMFont) {
	CCLabelBMFont* create(const char* content, const char* fontFile) {
		if (!enabled || !replaceAllText) return CCLabelBMFont::create(content, fontFile);
		std::string str(content);
		constexpr std::string_view replacement = "*";
		for (auto const& find : blacklist) {
			size_t pos = 0;
			while ((pos = geode::utils::string::toLower(str).find(find, pos)) != std::string::npos) {
				str.replace(pos, find.length(), replacement);
				pos += replacement.length();
			}
		}
		return CCLabelBMFont::create(str.c_str(), fontFile);
	}
	void setString(const char* newContent, bool needUpdateLabel) {
		if (!enabled || !replaceAllText) return CCLabelBMFont::setString(newContent, needUpdateLabel);
		std::string str(newContent);
		constexpr std::string_view replacement = "*";
		for (auto const& find : blacklist) {
			size_t pos = 0;
			while ((pos = geode::utils::string::toLower(str).find(find, pos)) != std::string::npos) {
				str.replace(pos, find.length(), replacement);
				pos += replacement.length();
			}
		}
		CCLabelBMFont::setString(str.c_str(), needUpdateLabel);
	}
};

class $modify(MyProfilePage, ProfilePage) {
	struct Fields {
		std::string reason;
	};
	void closeProfileWrapper(float dt) {
		std::string username = static_cast<std::string>(m_score->m_userName);
		std::string reason = m_fields->reason;
		if (CCScene::get()->getChildByType<ProfilePage>(0)) ProfilePage::onClose(nullptr);
		FLAlertLayer::create("Anti5ix5even", fmt::format("You are unable to view <cy>{}</c>'s profile:\n<c_>{}</c>", username, reason), "I Understand")->show();
	}
	void closeProfilePage(const std::string& reason) {
		m_fields->reason = reason;
		this->scheduleOnce(schedule_selector(MyProfilePage::closeProfileWrapper), .5f);
	}
	void loadPageFromUserInfo(GJUserScore* userInfo) {
		ProfilePage::loadPageFromUserInfo(userInfo);
		if (!enabled) return;
		if (closeProfilesForID && valueHasTheTwoDigits(userInfo->m_accountID)) return MyProfilePage::closeProfilePage(fmt::format("User's</c> <cj>Account ID</c> <c_>contains \"The Two Digits\" consecutively:</c> <cl>{}</c><c_>.", userInfo->m_accountID));
		if (closeProfilesForUserID && valueHasTheTwoDigits(userInfo->m_userID)) return MyProfilePage::closeProfilePage(fmt::format("User's</c> <cj>User ID</c> <c_>contains \"The Two Digits\" consecutively:</c> <cl>{}</c><c_>.", userInfo->m_userID));
		if (closeProfilesForGlobalRank && valueHasTheTwoDigits(userInfo->m_globalRank)) return MyProfilePage::closeProfilePage(fmt::format("User's</c> <cj>global rank</c> <c_>contains \"The Two Digits\" consecutively:</c> <cl>{}</c><c_>.", userInfo->m_globalRank));
		if (closeProfilesForAnyStats) {
			const auto [offendingStat, offendingStatValue] = checkForOffendingGJUSStats(userInfo);
			if (offendingStat != "unknown" && offendingStatValue > 0) return MyProfilePage::closeProfilePage(fmt::format("User's</c> {} <c_>stat contains \"The Two Digits\" consecutively:</c> <cl>{}</c><c_>.</c><c_>", offendingStat, offendingStatValue));
		}
		if (closeProfilesForUsername && geode::utils::string::containsAny(geode::utils::string::toLower(static_cast<std::string>(userInfo->m_userName)), blacklist)) return MyProfilePage::closeProfilePage("Username is forbidden.");
	}
};

class $modify(MyLevelListLayer, LevelListLayer) {
	struct Fields {
		std::string reason;
	};
	void scheduledLeave(float dt) {
		geode::createQuickPopup("Anti5ix5even", fmt::format( "You are unable to view the <cy>{}</c> level list:\n<c_>{}</c>", static_cast<std::string>(this->m_levelList->m_listName), m_fields->reason), "I Understand", nullptr, [this](FLAlertLayer* alert, bool close) {
			if (!CCScene::get()->getChildByType<LevelListLayer>(0)) return;
			LevelListLayer::onBack(nullptr);
		})->show();
	}
	void leaveFor(const std::string& reason) {
		if (!enabled || !this->m_levelList) return;
		m_fields->reason = reason;
		this->scheduleOnce(schedule_selector(MyLevelListLayer::scheduledLeave), 1.f);
	}
	void onEnter() {
		LevelListLayer::onEnter();
		if (!enabled || !m_levelList || m_levelList->m_listType == GJLevelType::Editor) return;
		if (!m_list || !m_list->m_listView || !m_list->m_listView->m_tableView || !m_list->m_listView->m_tableView->m_cellArray || !typeinfo_cast<LevelCell*>(m_list->m_listView->m_tableView->m_cellArray->objectAtIndex(0))) return;

		if (noLevelListsWithOffendingRewardCounts && (isEitherSixOrSeven(m_levelList->m_diamonds) || valueHasTheTwoDigits(m_levelList->m_diamonds))) {
			return MyLevelListLayer::leaveFor(fmt::format("Level List ID's</c> <cl>diamond rewards</c> <c_>contains \"The Two Digits\" consecutively:</c> <cl>{} diamonds</c><c_>.</c><c_>", m_levelList->m_diamonds));
		}

		if (noLevelListsWithOffendingLevelClaimRequirements && (isEitherSixOrSeven(m_levelList->m_levelsToClaim) || valueHasTheTwoDigits(m_levelList->m_levelsToClaim))) {
			return MyLevelListLayer::leaveFor(fmt::format("Level List ID's</c> <cj>completion requirement</c> <c_>contains \"The Two Digits\" consecutively:</c> <cl>{} levels</c><c_>.</c><c_>", m_levelList->m_levelsToClaim));
		}

		const std::string& levelIDAsString = fmt::to_string(this->m_levelList->m_listID);
		if (noOffendingLevelListIDsConsecutively && containsBothDigitsConsecutively(levelIDAsString)) {
			return MyLevelListLayer::leaveFor(fmt::format("</c><cj>Level List's ID</c> <c_>contains \"The Two Digits\" consecutively:</c> <cl>{}</c><c_>.</c><c_>", levelIDAsString));
		}
		if (noOffendingLevelListIDsIndividually && containsBothDigitsIndividually(levelIDAsString)) {
			return MyLevelListLayer::leaveFor(fmt::format("</c><cj>Level List's ID</c> <c_>contains both of \"The Two Digits\" in individual spots:</c> <cl>{}</c><c_>.</c><c_>", levelIDAsString));
		}
		if (noOffendingLevelListIDsExclusiveOr && containsEitherDigitsExclusiveOr(levelIDAsString)) {
			return MyLevelListLayer::leaveFor(fmt::format("</c><cj>Level List's ID</c> <c_>contains either one of \"The Two Digits\" at least once:</c> <cl>{}</c><c_>.</c><c_>", levelIDAsString));
		}

		if (noLevelListsContainingName && geode::utils::string::containsAny(geode::utils::string::toLower(static_cast<std::string>(m_levelList->m_listName)), blacklist)) {
			return MyLevelListLayer::leaveFor("Level list</c> <cj>name</c> <c_>is forbidden.");
		}

		if (noHarderLevelsInsideLevelLists) {
			for (LevelCell* levelCell : CCArrayExt<LevelCell*>(m_list->m_listView->m_tableView->m_cellArray)) {
				if (!levelCell->m_level) continue;
				if (!isEitherSixOrSeven(levelCell->m_level->m_stars.value())) continue;
				return MyLevelListLayer::leaveFor(fmt::format("Level list contains at least one level with a forbidden star count:</c> <cj>{}</c><c_>.</c><c_>", levelCell->m_level->m_levelID.value()));
			}
		}
	}
};

class $modify(MyGJGarageLayer, GJGarageLayer) {
	struct Fields {
		std::string offendingStat;
		int offendingStatValue;
	};
	void closeGarageWrapper(float dt) {
		MyGJGarageLayer::closeGarage(fmt::format("Your</c> {} <c_>stat contains one of \"The Two Digits\" in some way:</c> <cl>{}</c><c_>.</c><c_>", m_fields->offendingStat, m_fields->offendingStatValue));
	}
	void closeGarage(const std::string& reason) {
		geode::createQuickPopup("Anti5ix5even", fmt::format("You are unable to access your own icon kit:\n<c_>{}</c>", reason), "I Understand", nullptr, [this](FLAlertLayer* alert, bool close) {
			if (!CCScene::get()->getChildByType<GJGarageLayer>(0) && !CCScene::get()->getChildByType<GaragePopup>(0)) return;
			if (this->getUserObject("raydeeux.yetanotherqolmod/from-pauselayer")) static_cast<GaragePopup*>(this->getParent())->keyBackClicked();
			else GJGarageLayer::onBack(nullptr);
		})->show();
	}
	bool init() {
		if (!GJGarageLayer::init()) return false;
		if (!enabled) return true;
		if (closeIconKitForStats) {
			const auto [offendingStat, offendingStatValue] = checkForOffendingGSMStats();
			if (offendingStat != "unknown" && offendingStatValue > 0) {
				m_fields->offendingStat = offendingStat;
				m_fields->offendingStatValue = offendingStatValue;
				this->scheduleOnce(schedule_selector(MyGJGarageLayer::closeGarageWrapper), 1.f);
				return true;
			}
		}
		return true;
	}
};