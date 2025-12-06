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
        int level;
        std::string imageFile;
        int upgradeCost;
        float upgradeTime;
        std::string description;
    };
    static TownHallConfig* getInstance();
    const LevelData* getLevel(int level) const;
    const LevelData* getNextLevel(int currentLevel) const;
    int getMaxLevel() const { return static_cast<int>(_levels.size()); }
    bool canUpgrade(int currentLevel) const;
    int getUpgradeCost(int currentLevel) const;

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
    virtual bool upgrade() override;

protected:
    virtual bool init(int level) override;
    virtual void updateAppearance() override;
    virtual std::string getImageForLevel(int level) const override;

private:
    TownHallBuilding() = default;
};
#endif // TOWN_HALL_BUILDING_H_