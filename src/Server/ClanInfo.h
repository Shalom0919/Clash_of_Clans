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
 * @brief 玩家上下文信息，存储单个在线玩家的所有状态数据。
 *
 * 该结构体在玩家登录时创建，用于跟踪玩家的连接状态、游戏数据
 * 以及匹配状态等信息。生命周期与玩家的网络连接绑定。
 *
 * @note 线程安全性：该结构体的修改应在持有 PlayerRegistry 互斥锁时进行。
 */
struct PlayerContext {
    // 网络连接信息
    SOCKET socket = INVALID_SOCKET;    ///< 玩家的网络套接字句柄

    // 玩家身份信息
    std::string playerId;              ///< 玩家唯一标识符（登录账号）
    std::string playerName;            ///< 玩家昵称（显示名称）
    std::string clanId;                ///< 所属部落ID，空字符串表示未加入部落

    // 游戏数据
    std::string mapData;               ///< 玩家地图数据（JSON格式）
    int trophies = 0;                  ///< 奖杯数量，用于匹配和排名
    int gold = 1000;                   ///< 金币数量
    int elixir = 1000;                 ///< 圣水数量

    // 匹配状态
    bool isSearchingMatch = false;     ///< 是否正在搜索匹配
    std::chrono::steady_clock::time_point matchStartTime;  ///< 匹配开始时间点
};

/**
 * @struct ClanInfo
 * @brief 部落信息，存储部落的基本属性和成员列表。
 *
 * 该结构体代表一个部落实体，包含部落的元数据和成员关系。
 * 由 ClanHall 类管理创建、修改和销毁。
 *
 * @note 成员列表 memberIds 的第一个元素通常是部落创建者（族长）。
 */
struct ClanInfo {
    // 部落身份信息
    std::string clanId;                  ///< 部落唯一标识符，格式："CLAN_xxx"
    std::string clanName;                ///< 部落名称（显示名称）
    std::string leaderId;                ///< 族长（创建者）的玩家ID
    std::string description;             ///< 部落描述文本

    // 成员管理
    std::vector<std::string> memberIds;  ///< 部落成员ID列表

    // 部落属性
    int clanTrophies = 0;                ///< 部落总奖杯数（所有成员奖杯之和）
    int requiredTrophies = 0;            ///< 加入部落所需的最低奖杯数
    bool isOpen = true;                  ///< 是否开放加入（true 表示任何人可加入）
};

/**
 * @struct MatchQueueEntry
 * @brief 匹配队列条目，存储等待匹配的玩家信息。
 *
 * 当玩家发起匹配请求时创建此条目，由 Matchmaker 使用。
 * 包含匹配所需的玩家信息和入队时间戳。
 *
 * @see Matchmaker::Enqueue()
 * @see Matchmaker::ProcessQueue()
 */
struct MatchQueueEntry {
    SOCKET socket;                       ///< 玩家套接字，用于发送匹配结果
    std::string playerId;                ///< 玩家ID
    int trophies;                        ///< 玩家奖杯数，用于匹配算法
    std::chrono::steady_clock::time_point queueTime;  ///< 入队时间，用于超时处理
};