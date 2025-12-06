#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "BaseBuilding.h" // for ResourceType
#include "cocos2d.h"
/**
 * @struct BuildingData
 * @brief 建筑配置数据
 */
struct BuildingData
{
    std::string name;       // 建筑名称
    std::string imageFile;  // 图片文件
    cocos2d::Size gridSize; // 占用网格大小
    float scaleFactor;      // 缩放系数
    int cost;               // 建造费用
    int buildTime;          // 建造时间（秒）
    ResourceType costType;  // 消耗资源类型
    BuildingData()
        : name(""), imageFile(""), gridSize(cocos2d::Size::ZERO), scaleFactor(1.0f), cost(0), buildTime(0),
          costType(ResourceType::kGold)
    {}
    BuildingData(const std::string& n, const std::string& img, const cocos2d::Size& size, float scale = 1.0f, int c = 0,
                 int time = 0, ResourceType cType = ResourceType::kGold)
        : name(n), imageFile(img), gridSize(size), scaleFactor(scale), cost(c), buildTime(time), costType(cType)
    {}
};