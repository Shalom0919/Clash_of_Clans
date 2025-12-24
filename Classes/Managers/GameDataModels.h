/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GameDataModels.h
 * File Function: 游戏数据模型定义（纯数据结构）
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>
#include <vector>

/**
 * @struct BuildingSerialData
 * @brief 建筑序列化数据（用于存储/传输）
 */
struct BuildingSerialData {
    std::string name;
    int level = 1;
    float gridX = 0.0f;
    float gridY = 0.0f;
    float gridWidth = 1.0f;
    float gridHeight = 1.0f;
};

/**
 * @struct ResourceData
 * @brief 资源数据
 */
struct ResourceData {
    int gold = 1000;
    int elixir = 1000;
    int darkElixir = 0;
    int gems = 0;
    int goldCapacity = 3000;
    int elixirCapacity = 3000;
};

/**
 * @struct PlayerProgressData
 * @brief 玩家进度数据
 */
struct PlayerProgressData {
    int trophies = 0;
    int townHallLevel = 1;
    std::string clanId;
    std::string playerId;
};

/**
 * @struct GameStateData
 * @brief 完整游戏状态
 */
struct GameStateData {
    ResourceData resources;
    PlayerProgressData progress;
    std::string troopInventoryJson;
    std::vector<BuildingSerialData> buildings;
};

/**
 * @struct AccountData
 * @brief 账户数据
 */
struct AccountData {
    std::string userId;
    std::string username;
    std::string password;
    std::string token;
    std::string assignedMapName = "map/Map1.png";
};

/**
 * @typedef AccountGameData
 * @brief 向后兼容别名
 */
using AccountGameData = GameStateData;