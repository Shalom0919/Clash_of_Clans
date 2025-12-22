/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GoblinUnit.h
 * File Function: 哥布林单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseUnit.h"

class GoblinUnit : public BaseUnit
{
public:
    static GoblinUnit* create(int level = 1);

    virtual UnitType getUnitType() const override { return UnitType::kGoblin; }
    virtual std::string getDisplayName() const override { return "哥布林"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;
};