#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "GridMap.h"
#include "HeroManager.h"
#include "BuildingData.h"

class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;

    // 删除默认构造函数或显式声明
    DraggableMapScene() = default;
    ~DraggableMapScene() = default;

    CREATE_FUNC(DraggableMapScene);
    void showWholeGrid(bool visible);

private:
    // 基本成员变量声明
    cocos2d::Size _visibleSize;
    float _currentScale;
    float _minScale;
    float _maxScale;

    cocos2d::Sprite* _mapSprite;
    GridMap* _gridMap;
    cocos2d::Rect _mapBoundary;
    cocos2d::Vec2 _lastTouchPos;
    cocos2d::Vec2 _dragStartPos;    // 拖动起始位置

    // 建造模式相关
    bool _isBuildingMode;
    bool _isDraggingBuilding;       // 是否正在拖动建筑
    cocos2d::Sprite* _ghostSprite;
    BuildingData _selectedBuilding;

    // UI元素
    cocos2d::ui::Button* _buildButton;
    cocos2d::ui::Button* _mapButton;
    cocos2d::ui::ListView* _buildingListUI;
    cocos2d::ui::ListView* _mapList;
    bool _isBuildingListVisible;
    bool _isMapListVisible;

    // 英雄管理器
    HeroManager* _heroManager;

    // 数据
    std::string _currentMapName;
    std::vector<std::string> _mapNames;
    std::vector<BuildingData> _buildingList;

    // 地图元素
    struct MapElement {
        cocos2d::Node* node;
        cocos2d::Vec2 localPosition;
    };
    std::vector<MapElement> _mapElements;

    // ==== 新增的辅助函数声明 ====

    // 显示建筑放置提示
    void showBuildingHint(const std::string& hint);

    // 计算建筑位置
    cocos2d::Vec2 calculateBuildingPosition(const cocos2d::Vec2& gridPos);

    // 初始化方法
    void setupMap();
    void setupUI();
    void setupTouchListener();
    void setupMouseListener();
    void initBuildingData();
    void createBuildingSelection();
    void createMapList();

    // 触摸事件
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    // 建造系统
    void startPlacingBuilding(const BuildingData& building);
    void placeBuilding(cocos2d::Vec2 gridPos);
    void cancelPlacing();
    void endPlacing();

    // UI交互
    void toggleBuildingSelection();
    void onBuildingItemClicked(cocos2d::Ref* sender, const BuildingData& building);
    void toggleMapList();
    void onMapButtonClicked(cocos2d::Ref* sender);
    void onMapItemClicked(cocos2d::Ref* sender);
    void switchMap(const std::string& mapName);

    // 地图操作
    void moveMap(const cocos2d::Vec2& delta);
    void ensureMapInBoundary();
    void zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint = cocos2d::Vec2::ZERO);
    void updateBoundary();

    // 地图元素管理
    void saveMapElementsState();
    void restoreMapElementsState();
    void createSampleMapElements();
    void updateMapElementsPosition();
};

#endif