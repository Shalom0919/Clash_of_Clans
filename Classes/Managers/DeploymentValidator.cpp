/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DeploymentValidator.cpp
 * File Function: 部队部署验证器实现
 * Author:        GitHub Copilot
 * Update Date:   2025/12/26
 * License:       MIT License
 ****************************************************************/
#include "DeploymentValidator.h"

USING_NS_CC;

DeploymentValidator::DeploymentValidator() : grid_map_(nullptr) {}

DeploymentValidator::~DeploymentValidator() {
    Clear();
}

void DeploymentValidator::Init(GridMap* grid_map,
                               const std::vector<BaseBuilding*>& buildings) {
    grid_map_ = grid_map;
    forbidden_grids_.clear();

    if (!grid_map_) {
        CCLOG("⚠️ DeploymentValidator::Init: grid_map 为空");
        return;
    }

    // 遍历所有建筑，收集禁用区域
    for (const auto* building : buildings) {
        if (!building) {
            continue;
        }

        Vec2 grid_pos = building->getGridPosition();
        Size grid_size = building->getGridSize();

        int start_x = static_cast<int>(grid_pos.x);
        int start_y = static_cast<int>(grid_pos.y);
        int width = static_cast<int>(grid_size.width);
        int height = static_cast<int>(grid_size.height);

        // 建筑占用的网格及其周围一格都是禁止部署区域
        for (int x = start_x - 1; x < start_x + width + 1; ++x) {
            for (int y = start_y - 1; y < start_y + height + 1; ++y) {
                if (IsValidGrid(x, y)) {
                    forbidden_grids_.insert(std::make_pair(x, y));
                }
            }
        }
    }

    CCLOG("📍 DeploymentValidator 初始化完成: 禁用网格数=%zu",
          forbidden_grids_.size());
}

void DeploymentValidator::Clear() {
    forbidden_grids_.clear();
    grid_map_ = nullptr;
}

bool DeploymentValidator::CanDeployAt(const cocos2d::Vec2& world_position) const {
    if (!grid_map_) {
        CCLOG("⚠️ DeploymentValidator::CanDeployAt: grid_map 为空");
        return false;
    }

    // 转换世界坐标到网格坐标
    Vec2 grid_pos = grid_map_->getGridPosition(world_position);
    int grid_x = static_cast<int>(grid_pos.x);
    int grid_y = static_cast<int>(grid_pos.y);

    return CanDeployAtGrid(grid_x, grid_y);
}

bool DeploymentValidator::CanDeployAtGrid(int grid_x, int grid_y) const {
    // 检查网格是否在禁用列表中
    auto it = forbidden_grids_.find(std::make_pair(grid_x, grid_y));
    return it == forbidden_grids_.end();
}

bool DeploymentValidator::IsValidGrid(int x, int y) const {
    if (!grid_map_) {
        return false;
    }
    return (x >= 0 && x < grid_map_->getGridWidth() &&
            y >= 0 && y < grid_map_->getGridHeight());
}
