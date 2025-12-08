/**
 * @file TownHallBuilding.h
 * @brief 大本营建筑类
 */
#ifndef TOWN_HALL_BUILDING_H_
#define TOWN_HALL_BUILDING_H_
#include "BaseBuilding.h"
#include <vector>
/**
 * @class TownHallConfig
 * @brief 大本营等级配置管理器（单例）
 */
class TownHallConfig
{
public:
    struct LevelData
    {
        int level;              // 等级
        int hitpoints;          // 生命值
        int upgradeCost;        // 升级费用（金币）
        float upgradeTime;      // 升级时间（秒）
        int experienceGained;   // 升级后获得的经验值
        int maxBuildings;       // 最大建筑数量
        int maxTraps;           // 最大陷阱数量
        std::string imageFile;  // 图片路径
        std::string description;// 描述
    };
    
    static TownHallConfig* getInstance();
    const LevelData* getLevel(int level) const;
    const LevelData* getNextLevel(int currentLevel) const;
    int getMaxLevel() const { return static_cast<int>(_levels.size()); }
    bool canUpgrade(int currentLevel) const;
    int getUpgradeCost(int currentLevel) const;
    
    // ==================== 建筑限制系统（预留接口）====================
    // TODO: 未来实现 - 检查某建筑在当前大本营等级下的最大等级
    int getMaxBuildingLevel(int townHallLevel, const std::string& buildingName) const;
    
    // ==================== 建筑解锁系统（预留接口）====================
    // TODO: 未来实现 - 检查某建筑是否在当前大本营等级下解锁
    bool isBuildingUnlocked(int townHallLevel, const std::string& buildingName) const;

private:
    TownHallConfig();
    void initialize();
    std::vector<LevelData> _levels;
};
/**
 * @class TownHallBuilding
 * @brief 大本营建筑实体类
 */
class TownHallBuilding : public BaseBuilding
{
public:
    static TownHallBuilding* create(int level = 1);
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kTownHall; }
    virtual std::string getDisplayName() const override;
    virtual int getMaxLevel() const override;
    virtual int getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return kGold; }
    virtual float getUpgradeTime() const override;
    virtual std::string getUpgradeInfo() const override;
    virtual std::string getImageFile() const override;
    virtual bool canUpgrade() const override;
    // ❌ 移除：不再重写 upgrade()，使用 BaseBuilding 的统一升级流程

protected:
virtual bool init(int level) override;
virtual void onLevelUp() override;  // ✅ 重写升级完成后的逻辑
virtual void updateAppearance() override;
virtual std::string getImageForLevel(int level) const override;

private:
    TownHallBuilding() = default;
};
#endif // TOWN_HALL_BUILDING_H_