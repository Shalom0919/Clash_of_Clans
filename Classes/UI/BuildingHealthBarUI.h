#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingHealthBarUI.h
 * File Function: 建筑血条UI显示组件
 * Author:        刘相成
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "Buildings/BaseBuilding.h"
#include "cocos2d.h"

/**
 * @class BuildingHealthBarUI
 * @brief 建筑血条显示组件
 *
 * 功能：
 * 1. 显示建筑的当前生命值和最大生命值
 * 2. 实时更新血条显示（随着建筑受伤而下降）
 * 3. 血量满时隐藏，为零时建筑消失
 * 4. 支持不同战斗状态（攻击中、恢复中）
 */
class BuildingHealthBarUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建血条UI
     * @param building 关联的建筑对象
     */
    static BuildingHealthBarUI* create(BaseBuilding* building);

    virtual bool init(BaseBuilding* building);

    /**
     * @brief 更新血条显示
     */
    void update(float dt);

    /**
     * @brief 显示血条
     */
    void show();

    /**
     * @brief 隐藏血条
     */
    void hide();

    /**
     * @brief 设置血条是否始终可见（用于战斗时显示）
     */
    void setAlwaysVisible(bool always) { _alwaysVisible = always; }

    /**
     * @brief 检查关联的建筑是否已被销毁
     */
    bool isBuildingDestroyed() const;

private:
    BuildingHealthBarUI() = default;

    // ==================== 关联的建筑 ====================
    BaseBuilding* _building = nullptr;

    // ==================== 血条UI组件 ====================
    cocos2d::LayerColor* _healthBarBg   = nullptr; // 血条背景（红色）
    cocos2d::LayerColor* _healthBarFill = nullptr; // 血条填充（绿色）
    cocos2d::Label*      _healthLabel   = nullptr; // 血量文字（如 "100/200"）

    // ==================== 血条状态 ====================
    int   _lastHealthValue = -1;    // 上一帧的生命值（用于检测变化）
    float _hideTimer       = 0.0f;  // 隐藏计时器：血量恢复满后，3秒内自动隐藏血条
    bool  _isVisible       = false; // 血条是否可见
    bool  _alwaysVisible   = false; // 是否始终可见（战斗时为true）

    // ==================== 血条外观设置 ====================
    static constexpr float BAR_WIDTH  = 60.0f; // 血条宽度
    static constexpr float BAR_HEIGHT = 8.0f;  // 血条高度
    static constexpr float HIDE_DELAY = 3.0f;  // 血量满后多少秒隐藏血条
    static constexpr float OFFSET_Y   = 20.0f; // 血条相对于建筑的高度偏移
};