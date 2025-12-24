/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyCampBuilding.cpp
 * File Function: 军营建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ArmyCampBuilding.h"
#include "Managers/TroopInventory.h"
#include "Unit/UnitFactory.h"

USING_NS_CC;

ArmyCampBuilding* ArmyCampBuilding::create(int level)
{
    ArmyCampBuilding* building = new (std::nothrow) ArmyCampBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool ArmyCampBuilding::init(int level)
{
    if (!initWithType(BuildingType::kArmyCamp, level))
        return false;

    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);

    // 初始化时增加人口容量
    int housingSpace = getHousingSpace();
    ResourceManager::getInstance().addCapacity(kTroopPopulation, housingSpace);

    initHealthBarUI();
    return true;
}

int ArmyCampBuilding::getHousingSpace() const
{
    // 从配置中获取人口容量（存储在 resourceCapacity 字段）
    return _config.resourceCapacity;
}

void ArmyCampBuilding::onLevelUp()
{
    BuildingConfigData prevConfig = getStaticConfig(BuildingType::kArmyCamp, _level - 1);
    int prevHousingSpace = prevConfig.resourceCapacity;

    BaseBuilding::onLevelUp();

    int currentHousingSpace = getHousingSpace();
    int addedCapacity = currentHousingSpace - prevHousingSpace;

    if (addedCapacity > 0)
    {
        ResourceManager::getInstance().addCapacity(kTroopPopulation, addedCapacity);
    }
}

void ArmyCampBuilding::addTroopDisplay(UnitType type)
{
    BaseUnit* troopUnit = UnitFactory::createUnit(type);
    if (!troopUnit)
        return;
    
    troopUnit->setScale(0.5f);
    
    int index = static_cast<int>(_troopSprites.size());
    Vec2 pos = getTroopDisplayPosition(index);
    troopUnit->setPosition(pos);
    troopUnit->PlayAnimation(UnitAction::kIdle, UnitDirection::kRight);
    
    this->addChild(troopUnit, 50);
    _troopSprites.push_back(troopUnit);
}

void ArmyCampBuilding::removeTroopDisplay(UnitType type)
{
    if (_troopSprites.empty())
        return;
    
    auto lastSprite = _troopSprites.back();
    lastSprite->removeFromParent();
    _troopSprites.pop_back();
    updateTroopPositions();
}

void ArmyCampBuilding::clearTroopDisplays()
{
    for (auto* sprite : _troopSprites)
    {
        if (sprite)
            sprite->removeFromParent();
    }
    _troopSprites.clear();
}

void ArmyCampBuilding::updateTroopPositions()
{
    for (size_t i = 0; i < _troopSprites.size(); ++i)
    {
        if (_troopSprites[i])
        {
            Vec2 pos = getTroopDisplayPosition(static_cast<int>(i));
            auto moveTo = MoveTo::create(0.3f, pos);
            _troopSprites[i]->runAction(moveTo);
        }
    }
}

Vec2 ArmyCampBuilding::getTroopDisplayPosition(int index) const
{
    float buildingWidth = this->getContentSize().width;
    float buildingHeight = this->getContentSize().height;

    int row = index / 6;
    int col = index % 6;

    float startX = buildingWidth * 0.1f;
    float startY = buildingHeight * 0.25f;
    float spacingX = buildingWidth * 0.15f;
    float spacingY = buildingHeight * 0.2f;

    return Vec2(startX + col * spacingX, startY - row * spacingY);
}

void ArmyCampBuilding::refreshDisplayFromInventory()
{
    clearTroopDisplays();
    
    auto& troopInv = TroopInventory::getInstance();
    const auto& allTroops = troopInv.getAllTroops();
    
    std::vector<UnitType> displayOrder = {
        UnitType::kBarbarian,
        UnitType::kArcher,
        UnitType::kGoblin,
        UnitType::kGiant,
        UnitType::kWallBreaker
    };
    
    for (UnitType type : displayOrder)
    {
        auto it = allTroops.find(type);
        if (it != allTroops.end() && it->second > 0)
        {
            int count = std::min(it->second, getHousingSpace());
            for (int i = 0; i < count; ++i)
            {
                addTroopDisplay(type);
            }
        }
    }
}
