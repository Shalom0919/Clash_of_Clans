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
#include "GridMap.h" // <--- 1. 引入头文件
#include <unordered_map>
#include <string>
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
    cocos2d::Size _mapSize;
    float _tileSize;
    cocos2d::DrawNode* _gridNode; // 用于画全屏淡网格
    cocos2d::DrawNode* _baseNode; // 用于画当前鼠标下的绿色底座

    GridMap* _gridMap; // <--- 2. 新增网格对象指针
    cocos2d::Vec2 _gridStartDefault; // 存储网格默认起点，供重置使用

    // 每张地图的校准配置
    struct MapConfig {
        float scale; // 显示缩放/压缩系数
        cocos2d::Vec2 startPixel; // 对齐坐标（地图本地像素）
        float tileSize; // 小格子尺寸（像素）
    };

    std::unordered_map<std::string, MapConfig> _mapConfigs;

    // --- 建造模式相关 ---
    bool _isBuildingMode;           // 是否处于建造状态
    cocos2d::Sprite* _ghostSprite;  // 跟随鼠标的半透明建筑

    // 开始建造（点击UI按钮触发）
    void startPlacingBuilding();
    // 确认建造（点击地图触发）
    void placeBuilding(cocos2d::Vec2 gridPos);
    // 取消建造
    void cancelPlacing();
    // 结束建造（统一的退出建造模式接口）
    void endPlacing();

    cocos2d::Vec2 _lastTouchPos;
    cocos2d::Sprite* _mapSprite;
    cocos2d::Vec2 _velocity;
    cocos2d::Rect _mapBoundary;
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