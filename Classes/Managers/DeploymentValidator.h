/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DeploymentValidator.h
 * File Function: 部队部署验证器 - 检查部署位置是否有效
 * Author:        GitHub Copilot
 * Update Date:   2025/12/26
 * License:       MIT License
 ****************************************************************/
#ifndef DEPLOYMENT_VALIDATOR_H_
#define DEPLOYMENT_VALIDATOR_H_

#include "Buildings/BaseBuilding.h"
#include "GridMap.h"
#include "cocos2d.h"

#include <set>
#include <utility>
#include <vector>

/**
 * @class DeploymentValidator
 * @brief 部队部署验证器
 *
 * 负责验证部队部署位置是否有效。
 * 规则：不能在建筑物占用的网格及其周围一格内部署部队。
 */
class DeploymentValidator {
 public:
    /**
     * @brief 构造函数
     */
    DeploymentValidator();

    /**
     * @brief 析构函数
     */
    ~DeploymentValidator();

    /**
     * @brief 初始化验证器
     * @param grid_map 网格地图引用
     * @param buildings 建筑列表
     */
    void Init(GridMap* grid_map, const std::vector<BaseBuilding*>& buildings);

    /**
     * @brief 清空禁用区域数据
     */
    void Clear();

    /**
     * @brief 检查给定世界坐标位置是否可以部署部队
     * @param world_position 世界坐标位置
     * @return 可以部署返回 true，否则返回 false
     */
    bool CanDeployAt(const cocos2d::Vec2& world_position) const;

    /**
     * @brief 检查给定网格坐标是否可以部署部队
     * @param grid_x 网格 X 坐标
     * @param grid_y 网格 Y 坐标
     * @return 可以部署返回 true，否则返回 false
     */
    bool CanDeployAtGrid(int grid_x, int grid_y) const;

    /**
     * @brief 获取所有禁用部署的网格坐标
     * @return 禁用网格集合
     */
    const std::set<std::pair<int, int>>& GetForbiddenGrids() const {
        return forbidden_grids_;
    }

    /**
     * @brief 获取网格地图引用
     * @return 网格地图指针
     */
    GridMap* GetGridMap() const { return grid_map_; }

 private:
    /**
     * @brief 检查网格坐标是否在有效范围内
     * @param x 网格 X 坐标
     * @param y 网格 Y 坐标
     * @return 有效返回 true，否则返回 false
     */
    bool IsValidGrid(int x, int y) const;

    GridMap* grid_map_;                              ///< 网格地图引用
    std::set<std::pair<int, int>> forbidden_grids_;  ///< 禁止部署的网格集合
};

#endif  // DEPLOYMENT_VALIDATOR_H_
