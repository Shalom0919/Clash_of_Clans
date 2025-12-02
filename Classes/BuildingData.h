#pragma once
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#include "cocos2d.h"
#include <string>  // 确保包含 string

struct BuildingData {
    std::string name;       // 建筑名称
    std::string imageFile;  // 图片文件名
    cocos2d::Size gridSize; // 占用格子数 (如 3x3, 2x2)
    float scaleFactor;      // 缩放比例（根据格子尺寸调整）
    float cost;            // 建造费用
    float buildTime;       // 建造时间

    // 默认构造函数
    BuildingData()
        : name(""), imageFile(""), gridSize(cocos2d::Size::ZERO),
        scaleFactor(1.0f), cost(0), buildTime(0) {
    }

    // 参数化构造函数
    BuildingData(const std::string& n, const std::string& img,
        const cocos2d::Size& size, float scale = 1.0f, float c = 0, float t = 0)
        : name(n), imageFile(img), gridSize(size),
        scaleFactor(scale), cost(c), buildTime(t) {
    }

    // 获取放置说明 - 修复版本
    std::string getPlacementInstructions() const {
        // 方法1：使用 std::to_string
        return name + " - " +
            std::to_string((int)gridSize.width) + "x" +
            std::to_string((int)gridSize.height) +
            " - 点击开始，拖动调整，再次点击确认";

        // 或者方法2：使用 cocos2d 的方法（如果你有正确的头文件）
        // 需要包含 "base/CCConsole.h" 或 "platform/CCStdC.h"
    }

    // 另一个辅助方法，用于显示尺寸信息
    std::string getSizeInfo() const {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%dx%d",
            (int)gridSize.width, (int)gridSize.height);
        return std::string(buffer);
    }

    // 获取完整的描述信息
    std::string getDescription() const {
        return name + " [" + getSizeInfo() + "] - 费用: " +
            std::to_string((int)cost);
    }
};