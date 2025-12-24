/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WarModels.h
 * File Function: 战争和战斗数据模型定义
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "ClanInfo.h"

#include <chrono>
#include <map>
#include <string>
#include <vector>

/**
 * @struct AttackResult
 * @brief 攻击结果数据
 */
struct AttackResult {
    std::string attackerId;    // 攻击者ID
    std::string defenderId;    // 防守者ID
    int starsEarned = 0;       // 获得的星数
    int goldLooted = 0;        // 掠夺的金币
    int elixirLooted = 0;      // 掠夺的圣水
    int trophyChange = 0;      // 奖杯变化
    std::string replayData;    // 回放数据
};

/**
 * @struct AttackRecord
 * @brief 部落战争中的攻击记录
 */
struct AttackRecord {
    std::string attackerId;      // 攻击者ID
    std::string attackerName;    // 攻击者名称
    int starsEarned = 0;         // 获得的星数
    float destructionRate = 0.0f;  // 摧毁率
    std::chrono::steady_clock::time_point attackTime;  // 攻击时间
};

/**
 * @struct PvpSession
 * @brief PVP战斗会话
 */
struct PvpSession {
    std::string attackerId;      // 攻击者ID
    std::string defenderId;      // 防守者ID
    std::vector<std::string> spectatorIds;  // 观战者ID列表
    std::string mapData;         // 地图数据
    std::vector<std::string> actionHistory;  // 操作历史（格式："elapsedMs;unitType|x|y"）
    std::chrono::steady_clock::time_point startTime;  // 战斗开始时间
    bool isActive = true;        // 是否活跃
};

/**
 * @struct ClanWarMember
 * @brief 部落战争中的成员信息
 */
struct ClanWarMember {
    std::string memberId;        // 成员ID
    std::string memberName;      // 成员名称
    std::string mapData;         // 地图数据
    int bestStars = 0;           // 被攻击获得的最高星数
    float bestDestructionRate = 0.0f;  // 被攻击的最高摧毁率
    std::vector<AttackRecord> attacksReceived;  // 收到的攻击记录
};

/**
 * @struct ClanWarSession
 * @brief 部落战争会话
 */
struct ClanWarSession {
    std::string warId;           // 战争唯一标识
    std::string clan1Id;         // 第一个部落ID
    std::string clan2Id;         // 第二个部落ID
    std::vector<ClanWarMember> clan1Members;  // 第一个部落的成员
    std::vector<ClanWarMember> clan2Members;  // 第二个部落的成员
    std::map<std::string, PvpSession> activeBattles;  // 活跃的战斗（以攻击者ID为键）
    int clan1TotalStars = 0;     // 第一个部落的总星数
    int clan2TotalStars = 0;     // 第二个部落的总星数
    std::chrono::steady_clock::time_point startTime;  // 战争开始时间
    std::chrono::steady_clock::time_point endTime;    // 战争结束时间
    bool isActive = true;        // 战争是否活跃
};