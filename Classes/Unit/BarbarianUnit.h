/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BarbarianUnit.h
 * File Function: 野蛮人单位类
 * Author:        薛毓哲
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseUnit.h"

class BarbarianUnit : public BaseUnit
{
public:
    static BarbarianUnit* create(int level = 1);

    virtual UnitType    getUnitType() const override { return UnitType::kBarbarian; }
    virtual std::string getDisplayName() const override { return "野蛮人"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;
};