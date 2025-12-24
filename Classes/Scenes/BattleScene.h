/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleScene.h
 * File Function: 战斗场景
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "Buildings/DefenseBuilding.h"
#include "Managers/BattleManager.h"
#include "Managers/GameDataModels.h"
#include "Managers/ReplaySystem.h"
#include "UI/BattleUI.h"
#include "Unit/UnitTypes.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <map>
#include <string>
#include <vector>

// 前向声明
class BuildingManager;
class GridMap;
class BaseBuilding;

/**
 * @class BattleScene
 * @brief 战斗场景 - 异步多人游戏攻击场景
 *
 * 功能：
 * 1. 加载敌方基地布局
 * 2. 部署己方士兵进行攻击
 * 3. 计算战斗结果（星数、掠夺资源）
 * 4. 支持PVP模式和观战模式
 */
class BattleScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建战斗场景
     */
    static cocos2d::Scene* createScene();

    CREATE_FUNC(BattleScene);

    /**
     * @brief 创建战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     */
    static BattleScene* createWithEnemyData(const GameStateData& enemyData);

    /**
     * @brief 创建战斗场景（带敌方数据和用户ID）
     * @param enemyData 敌方玩家的基地数据
     * @param enemyUserId 敌方玩家ID
     */
    static BattleScene* createWithEnemyData(const GameStateData& enemyData, const std::string& enemyUserId);

    /**
     * @brief 创建战斗回放场景
     * @param replayDataStr 序列化的回放数据
     */
    static BattleScene* createWithReplayData(const std::string& replayDataStr);

    virtual bool init() override;
    virtual bool initWithEnemyData(const GameStateData& enemyData);
    virtual bool initWithEnemyData(const GameStateData& enemyData, const std::string& enemyUserId);
    virtual bool initWithReplayData(const std::string& replayDataStr);

    virtual void update(float dt) override;
    virtual void onEnter() override;
    virtual void onExit() override;

    /**
     * @brief 设置PVP模式
     * @param isAttacker 是否为攻击方
     */
    void setPvpMode(bool isAttacker);

    /**
     * @brief 设置观战模式
     * @param attackerId 攻击方ID
     * @param defenderId 防守方ID
     * @param elapsedMs 已经过的时间（毫秒）
     * @param history 历史操作记录
     */
    void setSpectateMode(const std::string& attackerId, 
                         const std::string& defenderId,
                         int64_t elapsedMs,
                         const std::vector<std::string>& history);

    /**
     * @brief 设置观战历史记录（向后兼容）
     * @param history 历史操作记录
     */
    void setSpectateHistory(const std::vector<std::string>& history);

private:
    BattleScene();
    ~BattleScene();

    // ==================== 场景元素 ====================
    cocos2d::Size    _visibleSize;
    cocos2d::Sprite* _mapSprite       = nullptr;
    GridMap*         _gridMap         = nullptr;
    BuildingManager* _buildingManager = nullptr;
    BattleUI*        _battleUI        = nullptr;
    BattleManager*   _battleManager   = nullptr;

    // ==================== 触摸控制相关 ====================
    cocos2d::Vec2 _lastTouchPos;
    bool          _isDragging = false;
    float         _timeScale  = 1.0f;

    // 多点触控缩放
    std::map<int, cocos2d::Vec2> _activeTouches;
    bool                         _isPinching        = false;
    float                        _prevPinchDistance = 0.0f;

    // ==================== 士兵部署数据 ====================
    UnitType _selectedUnitType = UnitType::kBarbarian;

    // ==================== 初始化方法 ====================
    void setupMap();
    void setupUI();
    void setupTouchListeners();

    // ==================== 交互逻辑 ====================
    void onTroopSelected(UnitType type);
    void returnToMainScene();
    void toggleSpeed();

    // ==================== 地图控制 ====================
    cocos2d::Rect _mapBoundary;
    void          updateBoundary();
    void          ensureMapInBoundary();

    // ==================== 战斗模式血条管理 ====================
    void enableAllBuildingsBattleMode();
    void disableAllBuildingsBattleMode();

    // ==================== 观战历史回放 ====================
    void replaySpectateHistory();

    // ==================== PVP/观战状态 ====================
    bool        _isPvpMode      = false;    ///< 是否为PVP模式
    bool        _isAttacker     = false;    ///< 是否为攻击方
    bool        _isSpectateMode = false;    ///< 是否为观战模式
    std::string _spectateAttackerId;        ///< 观战时的攻击方ID
    std::string _spectateDefenderId;        ///< 观战时的防守方ID
    int64_t     _spectateElapsedMs = 0;     ///< 观战时已经过的时间
    std::vector<std::string> _spectateHistory;  ///< 观战历史操作
    bool        _historyReplayed = false;   ///< 历史是否已回放
};

#endif // __BATTLE_SCENE_H__#endif // __BATTLE_SCENE_H__