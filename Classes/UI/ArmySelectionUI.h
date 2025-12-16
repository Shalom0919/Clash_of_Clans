//
#pragma once
#ifndef __ARMY_SELECTION_UI_H__
#define __ARMY_SELECTION_UI_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Unit/unit.h" // 引入 UnitType
#include <string>
#include <functional>
#include <map>

/**
 * @typedef TroopDeploymentMap
 * @brief 存储用户选择的部署军队数量：<UnitType, count>
 */
using TroopDeploymentMap = std::map<UnitType, int>;

/**
 * @class ArmySelectionUI
 * @brief 军队选择UI - 攻击前选择要部署的军队
 */
class ArmySelectionUI : public cocos2d::Layer
{
public:
    static ArmySelectionUI* create();
    virtual bool init() override;

    /**
     * @brief 设置确认回调 (携带选择的军队数量)
     * @param callback 回调函数
     */
    void setOnConfirmed(std::function<void(const TroopDeploymentMap&)> callback); // 🆕 修改签名

    /**
     * @brief 设置取消回调
     * @param callback 回调函数
     */
    void setOnCancelled(std::function<void()> callback);

    /**
     * @brief 显示UI
     */
    void show();

    /**
     * @brief 隐藏UI
     */
    void hide();

private:
    void createUI();

    /**
     * @brief 创建单个兵种选择卡片（横向布局）
     */
    void createTroopCard(cocos2d::ui::Layout* parent, UnitType type, int cardIndex); // 🆕 新增

    /**
     * @brief 加号/减号按钮点击事件
     */
    void onTroopCountChanged(UnitType type, int delta); // 🆕 新增

    /**
     * @brief 获取兵种名称（用于 UI 显示）
     */
    std::string getUnitName(UnitType type) const;

    /**
     * @brief 获取兵种图标路径
     */
    std::string getUnitIconPath(UnitType type) const;

private:
    std::function<void(const TroopDeploymentMap&)> _onConfirmed; // 🆕 修改签名
    std::function<void()> _onCancelled;

    cocos2d::Node* _container = nullptr;
    cocos2d::ui::Button* _confirmBtn = nullptr;
    cocos2d::ui::Button* _cancelBtn = nullptr;
    cocos2d::Size _visibleSize;

    // 🆕 军队数据
    TroopDeploymentMap _availableTroops; // 玩家当前可用的总库存 <UnitType, count>
    TroopDeploymentMap _selectedTroops;  // 玩家当前选择要部署的数量 <UnitType, count>

    // 🆕 UI 组件引用
    std::map<UnitType, cocos2d::Label*> _countLabels; // 存储数量标签的引用
};

#endif // __ARMY_SELECTION_UI_H__