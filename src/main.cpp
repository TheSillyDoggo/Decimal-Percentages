#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/utils/cocos.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>

using namespace geode::prelude;

float getPercentageForLevel(GJGameLevel* level, bool practice)
{
	if (level->m_levelType == GJLevelType::Editor)
	{
		auto str = fmt::format("percentage_{}_local_{}", practice ? "practice" : "normal", EditorIDs::getID(level));

		if (!Mod::get()->hasSavedValue(str))
			Mod::get()->setSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());

		return Mod::get()->getSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());
	}

	auto str = fmt::format("percentage_{}_{}", practice ? "practice" : "normal", level->m_levelID.value());

	if (!Mod::get()->hasSavedValue(str))
		Mod::get()->setSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());

	return Mod::get()->getSavedValue<float>(str, practice ? level->m_practicePercent : level->m_normalPercent.value());
}

std::string roundPercentage(float v)
{
	return numToString<float>(v, Mod::get()->getSettingValue<int64_t>("decimal-places"));
}

void savePercent(GJGameLevel* level, float percent, bool practice)
{
	if (level->m_levelType == GJLevelType::Editor)
	{
		auto str = fmt::format("percentage_{}_local_{}", practice ? "practice" : "normal", EditorIDs::getID(level));

		Mod::get()->setSavedValue<float>(str, percent);
		return;
	}

	auto str = fmt::format("percentage_{}_{}", practice ? "practice" : "normal", level->m_levelID.value());

	Mod::get()->setSavedValue<float>(str, percent);
}

class $modify(GJGameLevel)
{
	void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid)
	{
		GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);

		auto pl = PlayLayer::get();
		if (!pl) savePercent(this, percent, isPracticeMode);
		else if (pl->getCurrentPercent() > getPercentageForLevel(this, isPracticeMode))
			savePercent(this, PlayLayer::get()->getCurrentPercent(), isPracticeMode);

	}
};

class $modify (LevelInfoLayer)
{
	bool init(GJGameLevel* level, bool challenge)
	{
		if (!LevelInfoLayer::init(level, challenge))
			return false;

		if (auto normal = as<CCLabelBMFont*>(getChildByID("normal-mode-percentage")))
			normal->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, false))).c_str());

		if (auto practice = as<CCLabelBMFont*>(getChildByID("practice-mode-percentage")))
			practice->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, true))).c_str());

		return true;
	}
};

class $modify (PauseLayer)
{
	virtual void customSetup()
	{
		PauseLayer::customSetup();

		if (auto level = PlayLayer::get()->m_level)
		{
			if (auto normal = as<CCLabelBMFont*>(getChildByID("normal-progress-label")))
				normal->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, false))).c_str());

			if (auto practice = as<CCLabelBMFont*>(getChildByID("practice-progress-label")))
				practice->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, true))).c_str());
		}
	}
};

class $modify (LevelCell)
{
	void loadFromLevel(GJGameLevel* level)
	{
		LevelCell::loadFromLevel(level);

		if (auto percent = as<CCLabelBMFont*>(getChildByIDRecursive("percentage-label")))
			percent->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, false))).c_str());
	}
};

class $modify (LevelPage)
{
	void updateDynamicPage(GJGameLevel* level)
	{
		LevelPage::updateDynamicPage(level);

		if (level)
		{
			if (auto normal = as<CCLabelBMFont*>(getChildByID("normal-progress-label")))
				normal->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, false))).c_str());

			if (auto practice = as<CCLabelBMFont*>(getChildByID("practice-progress-label")))
				practice->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(level, true))).c_str());
		}
	}
};

class $modify (PlayLayer)
{
	void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5)
	{
		PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);

		const auto loader = Loader::get();
		if (const auto dst = loader->getLoadedMod("raydeeux.deathscreentweaks")) {
			if (dst->getSettingValue<bool>("enabled") && dst->getSettingValue<bool>("accuratePercent")) return;
		}

		loader->queueInMainThread([this]{
			for (auto child : CCArrayExt<CCNode*>(getChildren()))
			{
				if (child->getZOrder() == 100)
				{
					if (auto label = getChildOfType<CCLabelBMFont>(child, -1))
						label->setString(fmt::format("{}%", roundPercentage(getPercentageForLevel(m_level, false))).c_str());
				}
			}
		});
	}
};
