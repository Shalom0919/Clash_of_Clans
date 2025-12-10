#pragma once
#ifndef __ARMY_SELECTION_UI_H__
#define __ARMY_SELECTION_UI_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <string>
#include <functional>

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
     * @brief 设置确认回调
     * @param callback 回调函数
     */
    void setOnConfirmed(std::function<void()> callback);
    
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
    
private:
    std::function<void()> _onConfirmed;
    std::function<void()> _onCancelled;
    
    cocos2d::Node* _container = nullptr;
    cocos2d::ui::Button* _confirmBtn = nullptr;
    cocos2d::ui::Button* _cancelBtn = nullptr;
    cocos2d::Size _visibleSize;
};

#endif // __ARMY_SELECTION_UI_H__
