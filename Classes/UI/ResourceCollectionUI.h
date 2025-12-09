/**
 * @file ResourceCollectionUI.h
 * @brief 资源收集UI（显示可收集的资源图标和数量）
 */
 /****************************************************************
  * Project Name:  Clash_of_Clans
  * File Name:     WallBuilding.cpp
  * File Function: 资源收集类
  * Author:        刘相成
  * Update Date:   2025/12/09
  * License:       MIT License
  ****************************************************************/
#ifndef RESOURCE_COLLECTION_UI_H_
#define RESOURCE_COLLECTION_UI_H_

#include "cocos2d.h"
#include "../Managers/ResourceManager.h"

class ResourceBuilding;

class ResourceCollectionUI : public cocos2d::Node
{
public:
    static ResourceCollectionUI* create(ResourceBuilding* building);

    virtual bool init(ResourceBuilding* building);

    // 🆕 更新可收集状态（常驻显示资源图标和数量）
    void updateReadyStatus(int amount);

    // 🆕 播放收集反馈动画（飘字 + 消失）
    void playCollectionAnimation(int amount);

    // 检查触摸点是否在收集区域
    bool checkTouchInside(const cocos2d::Vec2& touchPos);

    // 执行收集逻辑（由 Manager 调用）
    void performCollection();

    // 是否可以被点击
    bool isClickable() const { return _isReadyToCollect; }

private:
    ResourceBuilding* _building = nullptr;
    cocos2d::Node* _iconContainer = nullptr;  // 包含图标和文字的容器
    cocos2d::Sprite* _resourceIcon = nullptr; // 资源图标
    cocos2d::Label* _amountLabel = nullptr;   // 资源数量

    bool _isReadyToCollect = false;           // 是否处于待收集状态
};

#endif // RESOURCE_COLLECTION_UI_H_