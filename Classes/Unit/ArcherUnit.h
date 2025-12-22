/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArcherUnit.h
 * File Function: 弓箭手单位类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseUnit.h"

class ArcherUnit : public BaseUnit
{
public:
    static ArcherUnit* create(int level = 1);

    virtual UnitType getUnitType() const override { return UnitType::kArcher; }
    virtual std::string getDisplayName() const override { return "弓箭手"; }

protected:
    virtual bool init(int level) override;
    virtual void loadAnimations() override;
};