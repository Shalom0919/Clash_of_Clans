/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingCapacityManager.cpp
 * File Function: 建筑增加资源容量类
 * Author:        刘相成、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "BuildingCapacityManager.h"
#include "Managers/ResourceManager.h"

USING_NS_CC;

BuildingCapacityManager& BuildingCapacityManager::getInstance()
{
    static BuildingCapacityManager instance;
    return instance;
}

BuildingCapacityManager::BuildingCapacityManager()
{
}

bool BuildingCapacityManager::init()
{
    if (!Node::init()) return false;
    return true;
}

void BuildingCapacityManager::registerOrUpdateBuilding(ResourceBuilding* building, bool added)
{
    if (!building)
        return;
    
    if (!building->isStorage())
        return;

    ResourceType type = building->getResourceType();
    
    // 确保资源类型正确
    if (type != ResourceType::kGold && type != ResourceType::kElixir)
        return;
    
    // 根据建筑名称二次验证资源类型
    std::string displayName = building->getDisplayName();
    ResourceType expectedType = type;
    
    if (displayName.find("金币仓库") != std::string::npos || 
        displayName.find("Gold Storage") != std::string::npos)
    {
        expectedType = ResourceType::kGold;
    }
    else if (displayName.find("圣水仓库") != std::string::npos || 
             displayName.find("Elixir Storage") != std::string::npos)
    {
        expectedType = ResourceType::kElixir;
    }
    
    if (type != expectedType)
        type = expectedType;

    if (added)
    {
        // 检查是否已存在，避免重复添加
        bool found = false;
        for (auto* b : _storageBuildings[type])
        {
            if (b == building)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            _storageBuildings[type].push_back(building);
        }
    }
    else
    {
        auto& list = _storageBuildings[type];
        list.erase(std::remove(list.begin(), list.end(), building), list.end());
    }

    recalculateCapacity();
}

static const int BASE_CAPACITY = 3000;

void BuildingCapacityManager::recalculateCapacity()
{
    _currentTotalCapacity.clear();
    auto& resMgr = ResourceManager::getInstance();

    // 计算金币总容量
    int totalGold = BASE_CAPACITY;
    if (_storageBuildings.find(ResourceType::kGold) != _storageBuildings.end())
    {
        std::vector<ResourceBuilding*> validBuildings;
        for (auto* building : _storageBuildings[ResourceType::kGold])
        {
            if (building && building->getReferenceCount() > 0 && !building->isDestroyed())
            {
                if (building->getResourceType() == ResourceType::kGold)
                {
                    totalGold += building->getStorageCapacity();
                    validBuildings.push_back(building);
                }
            }
        }
        _storageBuildings[ResourceType::kGold] = validBuildings;
    }
    _currentTotalCapacity[ResourceType::kGold] = totalGold;
    resMgr.setResourceCapacity(ResourceType::kGold, totalGold);

    // 计算圣水总容量
    int totalElixir = BASE_CAPACITY;
    if (_storageBuildings.find(ResourceType::kElixir) != _storageBuildings.end())
    {
        std::vector<ResourceBuilding*> validBuildings;
        for (auto* building : _storageBuildings[ResourceType::kElixir])
        {
            if (building && building->getReferenceCount() > 0 && !building->isDestroyed())
            {
                if (building->getResourceType() == ResourceType::kElixir)
                {
                    totalElixir += building->getStorageCapacity();
                    validBuildings.push_back(building);
                }
            }
        }
        _storageBuildings[ResourceType::kElixir] = validBuildings;
    }
    _currentTotalCapacity[ResourceType::kElixir] = totalElixir;
    resMgr.setResourceCapacity(ResourceType::kElixir, totalElixir);
}

int BuildingCapacityManager::getTotalCapacity(ResourceType type) const
{
    auto it = _currentTotalCapacity.find(type);
    return it != _currentTotalCapacity.end() ? it->second : 0;
}

void BuildingCapacityManager::clearAllBuildings()
{
    _storageBuildings.clear();
    _currentTotalCapacity.clear();
    
    auto& resMgr = ResourceManager::getInstance();
    resMgr.setResourceCapacity(ResourceType::kGold, BASE_CAPACITY);
    resMgr.setResourceCapacity(ResourceType::kElixir, BASE_CAPACITY);
}