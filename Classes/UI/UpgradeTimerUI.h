#pragma once
#include "cocos2d.h"

class BaseBuilding;
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 建筑倒计时类
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
/**
 * @class UpgradeTimerUI
 * @brief 建筑升级倒计时UI
 * 显示：倒计时文本 + 绿色进度条（从左向右填充灰色背景）
 */
class UpgradeTimerUI : public cocos2d::Node
{
public:
    static UpgradeTimerUI* create(BaseBuilding* building);
    virtual bool init(BaseBuilding* building);
    virtual void update(float dt) override;

private:
    BaseBuilding* _building = nullptr;
    cocos2d::Sprite* _barBg = nullptr;   // 灰色背景
    cocos2d::Sprite* _barFill = nullptr; // 绿色填充
    cocos2d::Label* _timeLabel = nullptr;// 剩余时间

    // 辅助：创建纯色纹理
    cocos2d::Sprite* createColorSprite(const cocos2d::Size& size, const cocos2d::Color3B& color);
    std::string formatTime(float seconds);
};