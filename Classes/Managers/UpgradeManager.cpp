/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UpgradeManager.cpp
 * File Function: 建筑升级管理器实现
 * Author:        薛毓哲
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#include "UpgradeManager.h"
#include "ResourceManager.h"
#include "Buildings/BaseBuilding.h"
#include <algorithm>

USING_NS_CC;

UpgradeManager* UpgradeManager::_instance = nullptr;

UpgradeManager* UpgradeManager::getInstance()
{
    if (!_instance)
    {
        _instance = new (std::nothrow) UpgradeManager();
        if (_instance && _instance->init())
        {
            _instance->autorelease();
            _instance->retain();  // 保持单例不被释放
        }
        else
        {
            CC_SAFE_DELETE(_instance);
        }
    }
    return _instance;
}

UpgradeManager::UpgradeManager()
{
}

bool UpgradeManager::init()
{
    if (!Node::init())
        return false;
    
    // 启用每帧更新
    scheduleUpdate();
    
    CCLOG("✅ UpgradeManager 初始化成功");
    return true;
}

// ==================== 核心接口实现 ====================

bool UpgradeManager::canStartUpgrade(BaseBuilding* building, bool needBuilder)
{
    if (!building)
    {
        return false;  // 不输出日志，这是内部错误
    }
    
    // 检查是否已在升级队列中
    if (isUpgrading(building))
    {
        // ✅ 只在实际升级时输出日志（不在 UI 检查时）
        return false;
    }
    
    // 🎮 作弊模式：跳过工人检查
    if (!_cheatModeEnabled && needBuilder)
    {
        // 检查是否有空闲工人
        int availableBuilders = getAvailableBuilders();
        if (availableBuilders <= 0)
        {
            // ✅ 只在实际升级时输出日志（不在 UI 检查时）
            return false;
        }
    }
    
    return true;
}

bool UpgradeManager::startUpgrade(BaseBuilding* building, int cost, float time, bool needBuilder)
{
    // 注意：此方法假设资源已经被扣除，只负责创建升级任务
    if (!building)
    {
        CCLOG("❌ startUpgrade 失败：建筑指针为空");
        return false;
    }
    
    // 再次检查（防御性编程）
    if (isUpgrading(building))
    {
        CCLOG("❌ %s 已在升级队列中", building->getDisplayName().c_str());
        return false;
    }
    
    // 🎮 作弊模式：升级时间为0
    if (_cheatModeEnabled)
    {
        time = 0.0f;
        CCLOG("🎮 [作弊模式] 升级时间设为 0 秒");
    }
    
    // 创建升级任务
    UpgradeTask task(building, time, cost, needBuilder);
    _upgradeTasks.push_back(task);
    
    // 标记建筑为升级中
    building->setUpgrading(true);
    
    // ✅ 修复：避免重复显示等级（getDisplayName 已包含等级）
    // 提取建筑名称（去掉 "Lv.X" 部分）
    std::string displayName = building->getDisplayName();
    size_t lvPos = displayName.find(" Lv.");
    std::string buildingName = (lvPos != std::string::npos) 
        ? displayName.substr(0, lvPos) 
        : displayName;
    
    CCLOG("🔨 开始升级：%s Lv.%d → Lv.%d（升级时间：%.1f 秒，费用：%d，工人：%s）", 
          buildingName.c_str(),      // ✅ 只显示建筑名称（如"大本营"）
          building->getLevel(),       // 当前等级
          building->getLevel() + 1,   // 目标等级
          time, 
          cost,
          needBuilder ? "需要" : "不需要");
    
    return true;
}

bool UpgradeManager::cancelUpgrade(BaseBuilding* building)
{
    if (!building)
        return false;
    
    // 查找升级任务
    auto it = std::find_if(_upgradeTasks.begin(), _upgradeTasks.end(),
                           [building](const UpgradeTask& task) {
                               return task.building == building;
                           });
    
    if (it == _upgradeTasks.end())
    {
        CCLOG("❌ 未找到 %s 的升级任务", building->getDisplayName().c_str());
        return false;
    }
    
    // 退还部分资源（50%）
    int refund = it->cost / 2;
    ResourceManager::getInstance().AddResource(building->getUpgradeCostType(), refund);
    
    // 释放工人
    if (it->useBuilder)
    {
        releaseBuilder();
    }
    
    // 移除任务
    building->setUpgrading(false);
    _upgradeTasks.erase(it);
    
    CCLOG("❌ 取消升级：%s，退还 %d 资源", building->getDisplayName().c_str(), refund);
    return true;
}

bool UpgradeManager::finishUpgradeNow(BaseBuilding* building)
{
    if (!building)
        return false;
    
    // 查找升级任务
    auto it = std::find_if(_upgradeTasks.begin(), _upgradeTasks.end(),
                           [building](const UpgradeTask& task) {
                               return task.building == building;
                           });
    
    if (it == _upgradeTasks.end())
    {
        CCLOG("❌ 未找到 %s 的升级任务", building->getDisplayName().c_str());
        return false;
    }
    
    // 立即完成升级
    completeUpgrade(*it);
    _upgradeTasks.erase(it);
    
    CCLOG("⚡ 立即完成升级：%s", building->getDisplayName().c_str());
    return true;
}

bool UpgradeManager::isUpgrading(BaseBuilding* building) const
{
    if (!building)
        return false;
    
    return std::any_of(_upgradeTasks.begin(), _upgradeTasks.end(),
                       [building](const UpgradeTask& task) {
                           return task.building == building;
                       });
}

UpgradeTask* UpgradeManager::getUpgradeTask(BaseBuilding* building) const
{
    if (!building)
        return nullptr;
    
    auto it = std::find_if(_upgradeTasks.begin(), _upgradeTasks.end(),
                           [building](const UpgradeTask& task) {
                               return task.building == building;
                           });
    
    return (it != _upgradeTasks.end()) ? const_cast<UpgradeTask*>(&(*it)) : nullptr;
}

int UpgradeManager::getAvailableBuilders() const
{
    auto& resMgr = ResourceManager::getInstance();
    int totalBuilders = resMgr.GetResourceCount(kBuilder);
    int usedBuilders = 0;
    
    // 统计正在使用的工人数量
    for (const auto& task : _upgradeTasks)
    {
        if (task.useBuilder)
        {
            usedBuilders++;
        }
    }
    
    return totalBuilders - usedBuilders;
}

// ==================== 每帧更新 ====================

void UpgradeManager::update(float dt)
{
    if (_upgradeTasks.empty())
        return;
    
    // 遍历所有升级任务
    auto it = _upgradeTasks.begin();
    while (it != _upgradeTasks.end())
    {
        // 🎮 作弊模式：升级时间为0，立即完成
        if (_cheatModeEnabled && it->totalTime <= 0.0f)
        {
            completeUpgrade(*it);
            it = _upgradeTasks.erase(it);
            continue;
        }
        
        // 更新升级进度
        it->elapsedTime += dt;
        
        // 检查是否完成
        if (it->elapsedTime >= it->totalTime)
        {
            completeUpgrade(*it);
            it = _upgradeTasks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ==================== 私有方法 ====================

void UpgradeManager::completeUpgrade(UpgradeTask& task)
{
    if (!task.building)
        return;
    
    // 调用建筑的升级完成逻辑
    task.building->onUpgradeComplete();
    
    // 释放工人
    if (task.useBuilder)
    {
        releaseBuilder();
    }
    
    // 标记建筑为未升级状态
    task.building->setUpgrading(false);
    
    CCLOG("🎉 升级完成：%s 达到 Lv.%d", 
          task.building->getDisplayName().c_str(), 
          task.building->getLevel());
}

bool UpgradeManager::allocateBuilder()
{
    auto& resMgr = ResourceManager::getInstance();
    
    // 检查是否有空闲工人
    if (getAvailableBuilders() <= 0)
    {
        return false;
    }
    
    // 工人数量不变，只是标记为"正在使用"
    // 实际的工人计数由 UpgradeTask 的 useBuilder 字段管理
    return true;
}

void UpgradeManager::releaseBuilder()
{
    // 释放工人（实际上只是减少 _upgradeTasks 中的 useBuilder 计数）
    // ResourceManager 中的工人数量不变
}
