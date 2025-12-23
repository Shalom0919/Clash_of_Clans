/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UnitFactory.cpp
 * File Function: 单位工厂实现
 * Author:        赵崇治
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#include "UnitFactory.h"
#include "BarbarianUnit.h"
#include "ArcherUnit.h"
#include "GiantUnit.h"
#include "GoblinUnit.h"
#include "WallBreakerUnit.h"

BaseUnit* UnitFactory::createUnit(UnitType type, int level)
{
    switch (type)
    {
    case UnitType::kBarbarian:
        return BarbarianUnit::create(level);
    case UnitType::kArcher:
        return ArcherUnit::create(level);
    case UnitType::kGiant:
        return GiantUnit::create(level);
    case UnitType::kGoblin:
        return GoblinUnit::create(level);
    case UnitType::kWallBreaker:
        return WallBreakerUnit::create(level);
    default:
        CCLOG("❌ Unknown unit type: %d", static_cast<int>(type));
        return nullptr;
    }
}

int UnitFactory::getUnitPopulation(UnitType type)
{
    switch (type)
    {
    case UnitType::kBarbarian:
        return 1;
    case UnitType::kArcher:
        return 1;
    case UnitType::kGoblin:
        return 1;
    case UnitType::kGiant:
        return 5;
    case UnitType::kWallBreaker:
        return 2;
    default:
        return 1;
    }
}

std::string UnitFactory::getUnitName(UnitType type)
{
    switch (type)
    {
    case UnitType::kBarbarian:
        return "野蛮人";
    case UnitType::kArcher:
        return "弓箭手";
    case UnitType::kGiant:
        return "巨人";
    case UnitType::kGoblin:
        return "哥布林";
    case UnitType::kWallBreaker:
        return "炸弹人";
    default:
        return "未知兵种";
    }
}