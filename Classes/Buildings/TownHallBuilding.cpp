/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TownHallBuilding.cpp
 * File Function: 大本营建筑实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "TownHallBuilding.h"
#include "Managers/BuildingLimitManager.h"
#include "Managers/ResourceManager.h"

USING_NS_CC;

TownHallBuilding* TownHallBuilding::create(int level)
{
    TownHallBuilding* ret = new (std::nothrow) TownHallBuilding();
    if (ret && ret->init(level))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TownHallBuilding::init(int level)
{
    if (!initWithType(BuildingType::kTownHall, level))
        return false;

    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);
    
    BuildingLimitManager::getInstance()->updateLimitsFromTownHall(_level);
    initHealthBarUI();
    
    return true;
}

bool TownHallBuilding::canUpgrade() const
{
    return _level < getMaxLevel() && !_isUpgrading;
}

std::string TownHallBuilding::getUpgradeInfo() const
{
    if (!canUpgrade())
        return "大本营已满级";

    BuildingConfigData nextConfig = getStaticConfig(BuildingType::kTownHall, _level + 1);
    return StringUtils::format("升级到 %s\n需要: %d 金币", 
                                nextConfig.description.c_str(), 
                                nextConfig.upgradeCost);
}

void TownHallBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    BuildingLimitManager::getInstance()->updateLimitsFromTownHall(_level);
}

void TownHallBuilding::updateAppearance()
{
    std::string imageFile = getImageFile();
    auto texture = Director::getInstance()->getTextureCache()->addImage(imageFile);
    if (texture)
    {
        this->setTexture(texture);
        this->setName(getDisplayName());
    }
}

// ==================== 建筑限制系统 ====================

int TownHallBuilding::getMaxBuildingLevel(const std::string& buildingName) const
{
    // 建筑等级不能超过大本营等级
    return _level;
}

bool TownHallBuilding::isBuildingUnlocked(const std::string& buildingName) const
{
    // TODO: 实现建筑解锁逻辑
    return true;
}