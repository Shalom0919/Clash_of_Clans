/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseBuilding.cpp
 * File Function: 建筑基类实现
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#include "BaseBuilding.h"
USING_NS_CC;
bool BaseBuilding::init(int level)
{
    _level = level;
    return true;
}
bool BaseBuilding::init(int level, const std::string& imageFile)
{
    if (!Sprite::initWithFile(imageFile))
    {
        return false;
    }
    _level = level;
    return true;
}
bool BaseBuilding::canUpgrade() const
{
    if (isMaxLevel())
    {
        return false;
    }
    if (_isUpgrading)
    {
        return false;
    }
    auto& resMgr = ResourceManager::getInstance();
    int cost = getUpgradeCost();
    ResourceType costType = getUpgradeCostType();
    return resMgr.hasEnough(costType, cost);
}
bool BaseBuilding::upgrade()
{
    if (!canUpgrade())
    {
        if (_upgradeCallback)
        {
            _upgradeCallback(false, _level);
        }
        return false;
    }
    auto& resMgr = ResourceManager::getInstance();
    int cost = getUpgradeCost();
    ResourceType costType = getUpgradeCostType();
    if (!resMgr.consume(costType, cost))
    {
        if (_upgradeCallback)
        {
            _upgradeCallback(false, _level);
        }
        return false;
    }
    // 升级成功
    _level++;
    onLevelUp();
    if (_upgradeCallback)
    {
        _upgradeCallback(true, _level);
    }
    return true;
}
void BaseBuilding::onLevelUp()
{
    updateAppearance();
}
void BaseBuilding::updateAppearance()
{
    std::string newImage = getImageForLevel(_level);
    if (!newImage.empty())
    {
        this->setTexture(newImage);
    }
}