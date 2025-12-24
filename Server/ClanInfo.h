/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanInfo.h
 * File Function: 玩家和部落数据结构定义
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <WinSock2.h>

#include <chrono>
#include <string>
#include <vector>

/**
 * @struct PlayerContext
 * @brief 玩家上下文信息
 */
struct PlayerContext {
    SOCKET socket = INVALID_SOCKET;    // 玩家的网络套接字
    std::string playerId;              // 玩家唯一标识
    std::string playerName;            // 玩家昵称
    std::string clanId;                // 所属部落ID
    std::string mapData;               // 玩家地图数据
    int trophies = 0;                  // 奖杯数量
    int gold = 1000;                   // 金币数量
    int elixir = 1000;                 // 圣水数量
    bool isSearchingMatch = false;     // 是否正在搜索匹配
    std::chrono::steady_clock::time_point matchStartTime;  // 匹配开始时间
};

/**
 * @struct ClanInfo
 * @brief 部落信息
 */
struct ClanInfo {
    std::string clanId;                // 部落唯一标识
    std::string clanName;              // 部落名称
    std::string leaderId;              // 族长玩家ID
    std::string description;           // 部落描述
    std::vector<std::string> memberIds;  // 成员ID列表
    int clanTrophies = 0;              // 部落总奖杯数
    int requiredTrophies = 0;          // 加入所需奖杯数
    bool isOpen = true;                // 是否开放加入
};

/**
 * @struct MatchQueueEntry
 * @brief 匹配队列条目
 */
struct MatchQueueEntry {
    SOCKET socket;                     // 玩家套接字
    std::string playerId;              // 玩家ID
    int trophies;                      // 奖杯数量
    std::chrono::steady_clock::time_point queueTime;  // 入队时间
};