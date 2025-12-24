/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DefenseBuilding.h
 * File Function: 防御建筑类
 * Author:        薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef DEFENSE_BUILDING_H_
#define DEFENSE_BUILDING_H_

#include "BaseBuilding.h"
#include "cocos2d.h"

#include <string>
#include <vector>

class BaseUnit;
class BuildingHealthBarUI;

/**
 * @enum DefenseType
 * @brief 防御建筑类型枚举
 */
enum class DefenseType
{
    kCannon,       ///< 加农炮
    kArcherTower,  ///< 箭塔
    kWizardTower   ///< 法师塔
};

/**
 * @class DefenseBuilding
 * @brief 防御建筑类 - 可自动攻击敌方单位
 */
class DefenseBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建防御建筑
     * @param defenseType 防御类型
     * @param level 等级
     * @return DefenseBuilding* 防御建筑指针
     */
    static DefenseBuilding* create(DefenseType defenseType, int level);

    /**
     * @brief 创建防御建筑（自定义图片）
     * @param defenseType 防御类型
     * @param level 等级
     * @param imageFile 图片文件路径
     * @return DefenseBuilding* 防御建筑指针
     */
    static DefenseBuilding* create(DefenseType defenseType, int level, const std::string& imageFile);

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const override { return BuildingType::kDefense; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const override;

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const override;

    /** @brief 获取升级费用 */
    virtual int getUpgradeCost() const override;

    /** @brief 获取最大生命值 */
    virtual int getMaxHitpoints() const override { return _maxHitpoints; }

    /** @brief 获取升级资源类型 */
    virtual ResourceType getUpgradeCostType() const override { return ResourceType::kGold; }

    /** @brief 获取升级时间 */
    virtual float getUpgradeTime() const override;

    /** @brief 获取建筑描述 */
    virtual std::string getBuildingDescription() const override;

    /** @brief 获取指定等级的图片路径 */
    virtual std::string getImageForLevel(int level) const override;

    /**
     * @brief 每帧更新
     * @param dt 帧时间间隔
     */
    virtual void tick(float dt) override;

    /**
     * @brief 检测敌方士兵并自动选择目标
     * @param units 敌方单位列表
     */
    void detectEnemies(const std::vector<BaseUnit*>& units);

    /**
     * @brief 攻击目标
     * @param target 目标单位
     */
    virtual void attackTarget(BaseUnit* target) override;

    /**
     * @brief 发射炮弹并造成伤害
     * @param target 目标单位
     */
    void fireProjectile(BaseUnit* target);

    /** @brief 播放攻击动画 */
    void playAttackAnimation();

    /** @brief 检查是否在战斗模式中 */
    bool isBattleModeEnabled() const { return _battleModeEnabled; }

    /** @brief 显示攻击范围 */
    void showAttackRange();

    /** @brief 隐藏攻击范围 */
    void hideAttackRange();

    /**
     * @brief 旋转建筑朝向目标
     * @param targetPos 目标位置
     */
    void rotateToTarget(const cocos2d::Vec2& targetPos);

    /** @brief 获取防御建筑类型 */
    DefenseType getDefenseType() const { return _defenseType; }

protected:
    virtual bool init(DefenseType defenseType, int level);
    virtual bool init(DefenseType defenseType, int level, const std::string& imageFile);
    virtual void onLevelUp() override;

private:
    void initCombatStats();  ///< 初始化战斗属性

    /**
     * @brief 创建炮弹精灵
     * @return cocos2d::Sprite* 炮弹精灵
     */
    cocos2d::Sprite* createCannonballSprite();

    /**
     * @brief 创建箭矢精灵
     * @return cocos2d::Sprite* 箭矢精灵
     */
    cocos2d::Sprite* createArrowSprite();

    DefenseType _defenseType;            ///< 防御建筑类型
    std::string _customImagePath;        ///< 自定义图片路径
    std::string _customName;             ///< 自定义名称
    cocos2d::DrawNode* _rangeCircle = nullptr;  ///< 攻击范围圆圈
};

#endif // DEFENSE_BUILDING_H_