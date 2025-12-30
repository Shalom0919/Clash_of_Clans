/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseUnit.h
 * File Function: 单位基类 - 所有士兵的父类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#ifndef BASE_UNIT_H_
#define BASE_UNIT_H_

#include "Unit/CombatStats.h"
#include "Unit/UnitTypes.h"
#include "cocos2d.h"

#include <map>
#include <string>
#include <vector>

class BaseBuilding;
class UnitHealthBarUI;

// 动作 Tag 常量，用于区分不同类型的动作以便单独停止
constexpr int kAnimationTag = 1001;     ///< 动画动作 Tag
constexpr int kDamageEffectTag = 1002;  ///< 受击颜色效果 Tag

/**
 * @class BaseUnit
 * @brief 单位基类 - 定义所有士兵的通用接口和行为
 */
class BaseUnit : public cocos2d::Node {
 public:
    virtual ~BaseUnit();

    /**
     * @brief 每帧更新（完整更新，包括移动和攻击冷却）
     * @param dt 帧时间间隔
     * @note 当由 BattleManager 统一管理攻击冷却时，应使用 tickMovement
     */
    virtual void tick(float dt);

    /**
     * @brief 每帧移动更新（仅更新移动逻辑，不更新攻击冷却）
     * @param dt 帧时间间隔
     * @note 当攻击冷却由外部（如 BattleManager）管理时使用此方法
     */
    virtual void tickMovement(float dt);

    /**
     * @brief 移动到指定位置
     * @param target_pos 目标位置
     */
    void moveTo(const cocos2d::Vec2& target_pos);

    /**
     * @brief 沿路径移动
     * @param path 路径点列表
     */
    void moveToPath(const std::vector<cocos2d::Vec2>& path);

    /** @brief 停止移动 */
    void stopMoving();

    /** @brief 获取移动速度 */
    virtual float getMoveSpeed() const { return _moveSpeed; }

    /** @brief 是否正在移动 */
    bool isMoving() const { return _isMoving; }

    /** @brief 获取目标位置 */
    const cocos2d::Vec2& getTargetPosition() const { return _targetPos; }

    /**
     * @brief 攻击
     * @param useSecondAttack 是否使用第二攻击
     */
    virtual void attack(bool useSecondAttack = false);

    /**
     * @brief 受到伤害
     * @param damage 伤害值
     * @return bool 是否死亡
     */
    virtual bool takeDamage(float damage);

    /** @brief 死亡 */
    virtual void die();

    /** @brief 是否死亡 */
    bool isDead() const { return _isDead; }

    /** @brief 是否等待移除（已从父节点移除，等待清理） */
    bool isPendingRemoval() const { return _pendingRemoval; }

    /** @brief 标记为等待移除状态 */
    void markPendingRemoval() { _pendingRemoval = true; }

    /** @brief 设置攻击目标 */
    void setTarget(BaseBuilding* target) { _currentTarget = target; }

    /** @brief 获取攻击目标 */
    BaseBuilding* getTarget() const { return _currentTarget; }

    /** @brief 清除攻击目标 */
    void clearTarget() { _currentTarget = nullptr; }

    /**
     * @brief 检查是否在攻击范围内
     * @param targetPos 目标位置
     * @return bool 是否在范围内
     */
    bool isInAttackRange(const cocos2d::Vec2& targetPos) const;

    /**
     * @brief 更新攻击冷却
     * @param dt 帧时间间隔
     */
    void updateAttackCooldown(float dt);

    /** @brief 攻击是否就绪 */
    bool isAttackReady() const { return _attackCooldown <= 0.0f; }

    /** @brief 重置攻击冷却 */
    void resetAttackCooldown();

    /** @brief 获取单位类型 */
    virtual UnitType getUnitType() const = 0;

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const = 0;

    /** @brief 获取战斗属性 */
    CombatStats& getCombatStats() { return _combatStats; }

    /** @brief 获取战斗属性 (const) */
    const CombatStats& getCombatStats() const { return _combatStats; }

    /** @brief 获取当前生命值 */
    int getCurrentHP() const { return _combatStats.currentHitpoints; }

    /** @brief 获取最大生命值 */
    int getMaxHP() const { return _combatStats.maxHitpoints; }

    /** @brief 获取攻击伤害 */
    float getDamage() const { return _combatStats.damage; }

    /** @brief 获取攻击范围 */
    float getAttackRange() const { return _combatStats.attackRange; }

    /** @brief 获取等级 */
    int getLevel() const { return _unitLevel; }

    /** @brief 初始化血条UI */
    void initHealthBarUI();

    /** @brief 启用战斗模式 */
    void enableBattleMode();

    /** @brief 禁用战斗模式 */
    void disableBattleMode();

    /** @deprecated 使用小写命名的方法 */
    void     MoveTo(const cocos2d::Vec2& target_pos) { moveTo(target_pos); }
    void     MoveToPath(const std::vector<cocos2d::Vec2>& path) { moveToPath(path); }
    void     StopMoving() { stopMoving(); }
    void     Attack(bool useSecondAttack = false) { attack(useSecondAttack); }
    void     Die() { die(); }
    bool     IsDead() const { return isDead(); }
    void     PlayAnimation(UnitAction action, UnitDirection dir) { playAnimation(action, dir); }
    UnitType GetType() const { return getUnitType(); }
    float    GetMoveSpeed() const { return getMoveSpeed(); }

 protected:
    BaseUnit();

    /**
     * @brief 初始化单位
     * @param level 单位等级
     * @return bool 初始化是否成功
     */
    virtual bool init(int level);

    /** @brief 加载动画（子类实现） */
    virtual void loadAnimations() = 0;

    /**
     * @brief 播放动画
     * @param action 动作类型
     * @param dir 方向
     */
    void playAnimation(UnitAction action, UnitDirection dir);

    /**
     * @brief 添加动画到缓存
     * @param key 动画键
     * @param anim 动画对象
     */
    void addAnimation(const std::string& key, cocos2d::Animation* anim);

    /**
     * @brief 从帧添加动画
     */
    void addAnimFromFrames(const std::string& unitName, const std::string& key, 
                           int start, int end, float delay);

    /**
     * @brief 从文件添加动画
     */
    void addAnimFromFiles(const std::string& basePath, const std::string& namePattern, 
                          const std::string& key, int start, int end, float delay);

    virtual void onAttackBefore() {}   ///< 攻击前回调
    virtual void onAttackAfter() {}    ///< 攻击后回调
    virtual void onDeathBefore() {}    ///< 死亡前回调
    virtual void onTakeDamage(float damage) {}  ///< 受伤回调

    /**
     * @brief 计算移动方向
     * @param direction 方向向量
     * @return UnitDirection 单位方向
     */
    UnitDirection calculateDirection(const cocos2d::Vec2& direction);

 protected:
    cocos2d::Sprite* _sprite = nullptr;                      ///< 精灵
    std::map<std::string, cocos2d::Animation*> _animCache;   ///< 动画缓存

    bool _isMoving = false;                    ///< 是否正在移动
    cocos2d::Vec2 _targetPos;                  ///< 目标位置
    cocos2d::Vec2 _moveVelocity;               ///< 移动速度向量
    float _moveSpeed = 100.0f;                 ///< 移动速度
    UnitDirection _currentDir = UnitDirection::kRight;  ///< 当前方向
    std::vector<cocos2d::Vec2> _pathPoints;    ///< 路径点
    int _currentPathIndex = 0;                 ///< 当前路径索引

    CombatStats _combatStats;                  ///< 战斗属性
    BaseBuilding* _currentTarget = nullptr;    ///< 当前攻击目标
    float _attackCooldown = 0.0f;              ///< 攻击冷却
    int _unitLevel = 1;                        ///< 单位等级
    bool _isDead = false;                      ///< 是否死亡
    bool _pendingRemoval = false;              ///< 是否等待移除（防止野指针访问）

    UnitHealthBarUI* _healthBarUI = nullptr;   ///< 血条UI
    bool _battleModeEnabled = false;           ///< 战斗模式是否启用
};

#endif  // BASE_UNIT_H_