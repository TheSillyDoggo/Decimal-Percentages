#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <regex>

static const std::regex percentageRegex(R"(^(?:\d+(?:\.\d+)?\%)[^\n\d]*(\d+(?:\.\d+)?\%)$)", std::regex::optimize | std::regex::icase);

using namespace geode::prelude;

float getPercentageForLevel(GJGameLevel* level, bool practice = false) {
	if (level->m_normalPercent > 99 && !practice || level->m_practicePercent > 99 && practice) return 100.f;
	if (level->m_levelType == GJLevelType::Editor) {
		auto str = fmt::format("percentage_{}_local_{}", practice ? "practice" : "normal", EditorIDs::getID(level));

		if (!Mod::get()->hasSavedValue(str)) {
			Mod::get()->setSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());
		}

		return Mod::get()->getSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());
	}

	auto str = fmt::format("percentage_{}_{}", practice ? "practice" : "normal", level->m_levelID.value());

	if (!Mod::get()->hasSavedValue(str)) {
		Mod::get()->setSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());
	}

	return Mod::get()->getSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());
}

std::string roundPercentage(float v) {
	return numToString<float>(v, Mod::get()->getSettingValue<int64_t>("decimalPlaces"));
}

std::string decimalPercentAsCString(GJGameLevel *level, bool practice = false) {
	return fmt::format("{}%", roundPercentage(getPercentageForLevel(level, practice)));
}

CCLabelBMFont* getLabelByID(CCNode* parent, const std::string& nodeID) {
	const auto node = typeinfo_cast<CCLabelBMFont*>(parent->getChildByIDRecursive(nodeID));
	if (!node) return nullptr;
	return node;
}

void savePercent(GJGameLevel* level, float percent, bool practice) {
	if (level->m_levelType == GJLevelType::Editor) {
		auto str = fmt::format("percentage_{}_local_{}", practice ? "practice" : "normal", EditorIDs::getID(level));
		Mod::get()->setSavedValue<float>(str, percent);
		return;
	}
	auto str = fmt::format("percentage_{}_{}", practice ? "practice" : "normal", level->m_levelID.value());
	Mod::get()->setSavedValue<float>(str, percent);
}

class $modify(GJGameLevel) {
	void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid) {
		GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);

		auto pl = PlayLayer::get();
		if (!pl) {
			savePercent(this, percent, isPracticeMode);
		} else if (pl->getCurrentPercent() > getPercentageForLevel(this, isPracticeMode)) {
			savePercent(this, PlayLayer::get()->getCurrentPercent(), isPracticeMode);
		}
	}
};

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
	bool init(GJGameLevel* level, bool challenge) {
		if (!LevelInfoLayer::init(level, challenge)) return false;

		if (auto normal = getLabelByID(this, "normal-mode-percentage")) {
			normal->setString(decimalPercentAsCString(level, false).c_str());
		}

		if (auto practice = getLabelByID(this, "practice-mode-percentage")) {
			practice->setString(decimalPercentAsCString(level, true).c_str());
		}

		return true;
	}
};

class $modify(MyPauseLayer, PauseLayer) {
	virtual void customSetup() {
		PauseLayer::customSetup();

		auto level = PlayLayer::get()->m_level;
		if (!level) return;
		if (auto normal = getLabelByID(this, "normal-progress-label")) {
			normal->setString(decimalPercentAsCString(level, false).c_str());
		}

		if (auto practice = getLabelByID(this, "practice-progress-label")) {
			practice->setString(decimalPercentAsCString(level, true).c_str());
		}
	}
};

class $modify(MyLevelCell, LevelCell) {
	void loadFromLevel(GJGameLevel* level) {
		LevelCell::loadFromLevel(level);

		if (!level) return;
		if (auto percent = getLabelByID(this, "percentage-label")) {
			percent->setString(decimalPercentAsCString(level, false).c_str());
		}
	}
};

class $modify(MyLevelPage, LevelPage) {
	void updateDynamicPage(GJGameLevel* level) {
		LevelPage::updateDynamicPage(level);

		if (!level) return;
		if (auto normal = getLabelByID(this, "normal-progress-label")) {
			normal->setString(decimalPercentAsCString(level, false).c_str());
		}

		if (auto practice = getLabelByID(this, "practice-progress-label")) {
			practice->setString(decimalPercentAsCString(level, true).c_str());
		}
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
		PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);

		if (!m_level) return;
		const auto loader = Loader::get();
		if (const auto dst = loader->getLoadedMod("raydeeux.deathscreentweaks")) {
			if (dst->getSettingValue<bool>("enabled") && dst->getSettingValue<bool>("accuratePercent")) return;
		}

		loader->queueInMainThread([this]{
			for (auto child : CCArrayExt<CCNode*>(this->getChildren())) {
				if (child->getZOrder() != 100) continue;
				const auto label = getChildOfType<CCLabelBMFont>(child, -1);
				if (!label) continue;
				if (!std::string(label->getString()).ends_with("%")) continue;
				const auto dst = Loader::get()->getLoadedMod("raydeeux.deathscreentweaks");
				if (!dst->getSettingValue<bool>("enabled")) label->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(m_level, false))).c_str());
				else label->setString(fmt::format("{:.{}f}%", getCurrentPercent(), Mod::get()->getSettingValue<int64_t>("decimalPlaces")).c_str());
			}
		});
	}

	void updateProgressbar() {
		PlayLayer::updateProgressbar();
		if (!m_level) return;
		if (m_level->isPlatformer()) return;
		if (!m_percentageLabel) return;
		std::string percentLabelText = m_percentageLabel->getString();
		std::smatch match;
		bool matches = std::regex_match(percentLabelText, match, percentageRegex);
		log::info("percentLabelText: {} | matches: {} | match.size(): {}", percentLabelText, matches, match.size());
		if (!matches) return;
		if (match.empty() || match.size() > 2) return;
		const std::string latestBestMaybe = match[1].str();
		log::info("probably the new best? {}", latestBestMaybe);
		const std::string newLabelText = std::regex_replace(percentLabelText, std::regex(latestBestMaybe), decimalPercentAsCString(m_level));
		m_percentageLabel->setString(newLabelText.c_str());
	}
};