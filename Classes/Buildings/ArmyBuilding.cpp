/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.cpp
 * File Function: 军事建筑类实现
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#include "ArmyBuilding.h"
USING_NS_CC;
ArmyBuilding* ArmyBuilding::create(int level)
{
    ArmyBuilding* building = new (std::nothrow) ArmyBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}
bool ArmyBuilding::init(int level)
{
    if (!BaseBuilding::init(level, getImageForLevel(level)))
    {
        return false;
    }
    return true;
}
int ArmyBuilding::getUpgradeCost() const
{
    // 升级费用随等级递增
    static const int costs[] = {0, 1000, 2000, 4000, 8000, 15000, 30000, 60000, 120000, 200000};
    int idx = std::min(_level, getMaxLevel());
    return costs[idx];
}
float ArmyBuilding::getUpgradeTime() const
{
    // 升级时间（秒）
    static const int times[] = {0, 60, 300, 900, 3600, 7200, 14400, 28800, 57600, 86400};
    int idx = std::min(_level, getMaxLevel());
    return times[idx];
}
std::string ArmyBuilding::getBuildingDescription() const
{
    return StringUtils::format("训练容量: %d\n训练速度: +%.0f%%", getTrainingCapacity(), getTrainingSpeedBonus() * 100);
}
int ArmyBuilding::getTrainingCapacity() const
{
    // 每级增加训练容量
    return 20 + (_level - 1) * 5;
}
float ArmyBuilding::getTrainingSpeedBonus() const
{
    // 每级增加5%训练速度
    return (_level - 1) * 0.05f;
}
void ArmyBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    CCLOG("ArmyBuilding upgraded to level %d, capacity: %d", _level, getTrainingCapacity());
}
std::string ArmyBuilding::getImageForLevel(int level) const
{
    // 根据等级返回不同的图片
    if (level <= 3)
        return "Barracks.png";
    else if (level <= 6)
        return "Barracks_2.png";
    else
        return "Barracks_3.png";
}