/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBreakerUnit.h
 * File Function: 炸弹人单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseUnit.h"

class WallBreakerUnit : public BaseUnit
{
public:
    static WallBreakerUnit* create(int level = 1);

    virtual UnitType getUnitType() const override { return UnitType::kWallBreaker; }
    virtual std::string getDisplayName() const override { return "炸弹人"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;
    
    // 炸弹人特殊行为：死亡时爆炸
    virtual void onDeathBefore() override;
};