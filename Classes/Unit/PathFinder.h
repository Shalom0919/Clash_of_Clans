/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PathFinder.h
 * File Function: A*寻路算法实现
 * Author:        刘相成、赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __PATH_FINDER_H__
#define __PATH_FINDER_H__

#include "GridMap.h"
#include "cocos2d.h"

#include <vector>

/**
 * @struct PathNode
 * @brief 寻路节点
 */
struct PathNode
{
    int x, y;              ///< 节点坐标
    int gCost;             ///< 起点到当前节点的代价
    int hCost;             ///< 当前节点到终点的估算代价
    PathNode* parent;      ///< 父节点

    /** @brief 获取总代价 */
    int fCost() const { return gCost + hCost; }

    /** @brief 比较运算符 */
    bool operator>(const PathNode& other) const { return fCost() > other.fCost(); }

    PathNode(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {}
};

/**
 * @class PathFinder
 * @brief A*寻路器（单例）
 */
class PathFinder
{
public:
    /**
     * @brief 获取单例实例
     * @return PathFinder& 单例引用
     */
    static PathFinder& getInstance();

    /**
     * @brief 核心A*寻路函数
     * @param gridMap 网格地图
     * @param startWorldUnit 起点世界坐标
     * @param endWorldTarget 终点世界坐标
     * @param ignoreWalls 是否忽略城墙
     * @return std::vector<cocos2d::Vec2> 路径点列表
     */
    std::vector<cocos2d::Vec2> findPath(GridMap* gridMap, const cocos2d::Vec2& startWorldUnit,
                                        const cocos2d::Vec2& endWorldTarget, bool ignoreWalls = false);

private:
    int getDistance(const PathNode* nodeA, const PathNode* nodeB);
    bool isValid(int x, int y, int width, int height);

    /**
     * @brief 检查两点之间是否有视线
     */
    bool hasLineOfSight(GridMap* gridMap, const cocos2d::Vec2& start, const cocos2d::Vec2& end, bool ignoreWalls);

    /**
     * @brief 路径平滑
     */
    std::vector<cocos2d::Vec2> smoothPath(GridMap* gridMap, const std::vector<cocos2d::Vec2>& rawPath,
                                          bool ignoreWalls);

    PathFinder() = default;
    ~PathFinder() = default;
    PathFinder(const PathFinder&) = delete;
    PathFinder& operator=(const PathFinder&) = delete;
};

#endif // __PATH_FINDER_H__