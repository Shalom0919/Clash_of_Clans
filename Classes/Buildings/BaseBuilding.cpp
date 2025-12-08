/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseBuilding.cpp
 * File Function: 建筑基类实现
 * Author:        赵崇治
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#include "BaseBuilding.h"
#include "Managers/UpgradeManager.h"
USING_NS_CC;
bool BaseBuilding::init(int level)
{
    _level = level;
    return true;
}
bool BaseBuilding::init(int level, const std::string& imageFile)
{
    if (!Sprite::initWithFile(imageFile))
    {
        return false;
    }
    _level = level;
    return true;
}
bool BaseBuilding::canUpgrade() const
{
    if (isMaxLevel())
    {
        return false;
    }
    if (_isUpgrading)
    {
        return false;
    }
    
    // ✅ 检查资源
    auto& resMgr = ResourceManager::getInstance();
    int cost = getUpgradeCost();
    ResourceType costType = getUpgradeCostType();
    if (!resMgr.hasEnough(costType, cost))
    {
        return false;
    }
    
    // ✅ 检查工人（UI 显示用）
    auto* upgradeMgr = UpgradeManager::getInstance();
    if (!upgradeMgr->canStartUpgrade(const_cast<BaseBuilding*>(this), true))
    {
        return false;
    }
    
    return true;
}
bool BaseBuilding::upgrade()
{
    if (!canUpgrade())
    {
        if (_upgradeCallback)
        {
            _upgradeCallback(false, _level);
        }
        return false;
    }
    
    auto& resMgr = ResourceManager::getInstance();
    int cost = getUpgradeCost();
    ResourceType costType = getUpgradeCostType();
    float time = getUpgradeTime();
    
    // 🔧 修复：先检查工人，再扣除资源，并输出详细日志
    auto* upgradeMgr = UpgradeManager::getInstance();
    
    // 检查工人是否足够（不扣资源）
    if (!upgradeMgr->canStartUpgrade(this, true))
    {
        // ✅ 详细的工人不足日志
        int availableBuilders = upgradeMgr->getAvailableBuilders();
        int totalBuilders = resMgr.GetResourceCapacity(kBuilder);
        int usedBuilders = upgradeMgr->getUpgradeQueueLength();
        
        if (upgradeMgr->isUpgrading(this))
        {
            CCLOG("❌ 升级失败：%s 已在升级队列中", getDisplayName().c_str());
        }
        else
        {
            CCLOG("❌ 升级失败：建筑工人不足！");
            CCLOG("   - 当前空闲工人：%d", availableBuilders);
            CCLOG("   - 总工人数：%d", totalBuilders);
            CCLOG("   - 正在使用：%d", usedBuilders);
        }
        
        if (_upgradeCallback)
        {
            _upgradeCallback(false, _level);
        }
        return false;
    }
    
    // 检查并扣除资源
    if (!resMgr.consume(costType, cost))
    {
        // ✅ 资源不足的准确提示
        std::string resName = (costType == ResourceType::kGold) ? "金币" : "圣水";
        CCLOG("❌ 升级失败：%s不足！", resName.c_str());
        CCLOG("   - 需要：%d", cost);
        CCLOG("   - 当前：%d", resMgr.GetResourceCount(costType));
        
        if (_upgradeCallback)
        {
            _upgradeCallback(false, _level);
        }
        return false;
    }
    
    // 启动升级倒计时（此时资源已扣除，工人已检查）
    if (!upgradeMgr->startUpgrade(this, cost, time, true))
    {
        // 升级失败，退还资源
        resMgr.addResource(costType, cost);
        CCLOG("❌ 启动升级失败，已退还 %d 资源", cost);
        if (_upgradeCallback)
        {
            _upgradeCallback(false, _level);
        }
        return false;
    }
    
    // ✅ 修复：避免重复显示等级
    std::string displayName = getDisplayName();
    size_t lvPos = displayName.find(" Lv.");
    std::string buildingName = (lvPos != std::string::npos) 
        ? displayName.substr(0, lvPos) 
        : displayName;
    
    CCLOG("✅ %s 开始升级：Lv.%d → Lv.%d（需要 %.0f 秒，费用 %d）", 
          buildingName.c_str(), _level, _level + 1, time, cost);
    
    return true;
}

void BaseBuilding::onUpgradeComplete()
{
    // 升级成功
    _level++;
    onLevelUp();
    
    if (_upgradeCallback)
    {
        _upgradeCallback(true, _level);
    }
}

float BaseBuilding::getUpgradeProgress() const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    auto* task = upgradeMgr->getUpgradeTask(const_cast<BaseBuilding*>(this));
    
    return task ? task->getProgress() : 0.0f;
}

float BaseBuilding::getUpgradeRemainingTime() const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    auto* task = upgradeMgr->getUpgradeTask(const_cast<BaseBuilding*>(this));
    
    return task ? task->getRemainingTime() : 0.0f;
}
void BaseBuilding::onLevelUp()
{
    updateAppearance();
}
void BaseBuilding::updateAppearance()
{
    std::string newImage = getImageForLevel(_level);
    if (!newImage.empty())
    {
        this->setTexture(newImage);
    }
}

// ==================== 生命值系统实现 ====================
void BaseBuilding::takeDamage(int damage)
{
    if (damage <= 0) return;
    
    _currentHitpoints -= damage;
    if (_currentHitpoints < 0)
    {
        _currentHitpoints = 0;
    }
    
    CCLOG("🔨 %s 受到 %d 点伤害！剩余生命值：%d/%d", 
          getDisplayName().c_str(), damage, _currentHitpoints, _maxHitpoints);
    
    // TODO: 播放受伤动画、音效等
    if (isDestroyed())
    {
        CCLOG("💥 %s 已被摧毁！", getDisplayName().c_str());
        // TODO: 播放摧毁动画
    }
}

void BaseBuilding::repair(int amount)
{
    if (amount <= 0) return;
    
    _currentHitpoints += amount;
    if (_currentHitpoints > _maxHitpoints)
    {
        _currentHitpoints = _maxHitpoints;
    }
    
    CCLOG("🔧 %s 修复 %d 点生命值！当前：%d/%d", 
          getDisplayName().c_str(), amount, _currentHitpoints, _maxHitpoints);
}