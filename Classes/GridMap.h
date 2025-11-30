#pragma once
#include "cocos2d.h"
#include <vector>

class GridMap : public cocos2d::Node
{
private:
    cocos2d::Size _mapSize;
    float _tileSize; // 小方格的尺寸
    cocos2d::DrawNode* _gridNode;
    cocos2d::DrawNode* _baseNode;

    // 冲突检测：true = 有建筑/障碍，false = 空地
    std::vector<std::vector<bool>> _collisionMap;
    int _gridWidth;  // 网格横向数量
    int _gridHeight; // 网格纵向数量

    // 网格起始点（在地图本地坐标系中，表示 grid (0,0) 的中心像素）
    cocos2d::Vec2 _startPixel;
    bool _gridVisible;

public:
    enum Corner { TOP_LEFT = 0, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER };

    static GridMap* create(const cocos2d::Size& mapSize, float tileSize);
    virtual bool init(const cocos2d::Size& mapSize, float tileSize);

    void showWholeGrid(bool visible);

    // 设置网格起始点（直接以像素位置指定）
    void setStartPixel(const cocos2d::Vec2& pixel);
    cocos2d::Vec2 getStartPixel() const;

    // 将起始点设置为贴图的某一角（方便对齐）
    void setStartCorner(Corner corner);

    // 核心坐标转换（基于 ISO 45度）
    cocos2d::Vec2 getGridPosition(cocos2d::Vec2 worldPosition);
    cocos2d::Vec2 getPositionFromGrid(cocos2d::Vec2 gridPos);

    // 【升级】支持传入建筑尺寸 (width x height)
    // gridPos: 建筑左上角（其实是ISO的Top角）所在的格子
    // size: 建筑占据多少个小格子，例如 Size(3, 3)
    void updateBuildingBase(cocos2d::Vec2 gridPos, cocos2d::Size size, bool isValid);

    void hideBuildingBase();

    // 【新增】冲突检测 API
    // 检查这个区域是否可以建造
    bool checkArea(cocos2d::Vec2 startGridPos, cocos2d::Size size);

    // 标记这个区域被占用
    void markArea(cocos2d::Vec2 startGridPos, cocos2d::Size size, bool occupied);
};