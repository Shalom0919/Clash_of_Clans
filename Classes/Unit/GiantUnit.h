/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GiantUnit.h
 * File Function: 巨人单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseUnit.h"

class GiantUnit : public BaseUnit
{
public:
    static GiantUnit* create(int level = 1);

    virtual UnitType getUnitType() const override { return UnitType::kGiant; }
    virtual std::string getDisplayName() const override { return "巨人"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;
};