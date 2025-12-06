/**
 * @file ResourceBuilding.h
 * @brief 资源生产建筑类（金矿、圣水收集器等）
 */
#ifndef RESOURCE_BUILDING_H_
#define RESOURCE_BUILDING_H_
#include "BaseBuilding.h"
/**
 * @class ResourceBuilding
 * @brief 资源生产建筑基类
 */
class ResourceBuilding : public BaseBuilding
{
public:
    static ResourceBuilding* create(ResourceType resourceType, int level = 1);
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kResource; }
    virtual std::string getDisplayName() const override;
    virtual int getMaxLevel() const override { return 15; }
    virtual int getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return kGold; }
    virtual std::string getImageFile() const override;
    virtual bool upgrade() override;
    virtual void tick(float dt) override;
    // ==================== 资源生产相关 ====================
    ResourceType getResourceType() const { return _resourceType; }
    int getProductionRate() const;
    int getStorageCapacity() const;
    int getCurrentStorage() const { return _currentStorage; }
    int collect();
    bool isStorageFull() const { return _currentStorage >= getStorageCapacity(); }

protected:
    ResourceBuilding() = default;
    virtual bool init(int level) override;
    virtual void updateAppearance() override;
    virtual std::string getImageForLevel(int level) const override;
    void showCollectHint();
    void hideCollectHint();

private:
    ResourceType _resourceType = kGold;
    int _currentStorage = 0;
    float _productionAccumulator = 0.0f;
    cocos2d::Label* _storageLabel = nullptr;
};
#endif // RESOURCE_BUILDING_H_