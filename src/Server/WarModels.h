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
 * @brief 普通攻击结果数据，记录单次攻击的完整结果信息。
 *
 * 用于普通匹配战斗的结果记录，包含资源掠夺、奖杯变化等信息。
 * 攻击结束后由客户端上报，服务器进行资源结算。
 *
 * @note 与部落战争中的 AttackRecord 不同，此结构体包含资源变化信息。
 * @see AttackRecord
 */
struct AttackResult {
    // 参与者信息
    std::string attackerId;    ///< 攻击者玩家ID
    std::string defenderId;    ///< 防守者玩家ID

    // 战斗结果
    int starsEarned = 0;       ///< 获得的星数（0-3）
    int goldLooted = 0;        ///< 掠夺的金币数量
    int elixirLooted = 0;      ///< 掠夺的圣水数量
    int trophyChange = 0;      ///< 奖杯变化值（可正可负）

    // 回放数据
    std::string replayData;    ///< 战斗回放数据（用于录像功能）
};

/**
 * @struct AttackRecord
 * @brief 部落战争中的攻击记录，用于统计部落战争成绩。
 *
 * 记录部落战争中单次攻击的结果，不包含资源变化（部落战无资源掠夺）。
 * 用于计算部落战争的胜负和成员贡献。
 *
 * @note 与普通战斗的 AttackResult 不同，此结构体专用于部落战争。
 * @see AttackResult
 * @see ClanWarSession
 */
struct AttackRecord {
    // 攻击者信息
    std::string attackerId;      ///< 攻击者玩家ID
    std::string attackerName;    ///< 攻击者名称（用于显示）

    // 战斗结果
    int starsEarned = 0;         ///< 获得的星数（0-3）
    float destructionRate = 0.0f;  ///< 摧毁率（0.0-1.0，例如 0.85 表示 85%）

    // 时间戳
    std::chrono::steady_clock::time_point attackTime;  ///< 攻击发生的时间点
};

/**
 * @struct PvpSession
 * @brief PVP 战斗会话，管理单场实时 PVP 战斗的状态。
 *
 * 当玩家发起 PVP 请求并被接受后创建此会话。会话记录攻防双方信息、
 * 地图数据、操作历史和观战者列表。用于实现实时战斗同步和观战功能。
 *
 * @note 以攻击者ID为键存储在 ArenaSession 或 ClanWarSession 中。
 * @note actionHistory 中的每条记录格式为 "elapsedMs;unitType|x|y"。
 *
 * @see ArenaSession
 * @see ClanWarSession
 */
struct PvpSession {
    // 参与者信息
    std::string attackerId;      ///< 攻击者玩家ID（会话的键）
    std::string defenderId;      ///< 防守者玩家ID

    // 观战者管理
    std::vector<std::string> spectatorIds;  ///< 观战者玩家ID列表

    // 战斗数据
    std::string mapData;         ///< 防守方地图数据（战斗开始时快照）
    std::vector<std::string> actionHistory;  ///< 操作历史记录，用于观战同步

    // 会话状态
    std::chrono::steady_clock::time_point startTime;  ///< 战斗开始时间点
    bool isActive = true;        ///< 会话是否活跃（false 表示战斗已结束）
};

/**
 * @struct ClanWarMember
 * @brief 部落战争中的成员信息，跟踪单个成员在战争中的状态。
 *
 * 存储部落战争参与者的信息，包括地图数据和被攻击记录。
 * 用于显示战争成员列表和统计战争结果。
 *
 * @note bestStars 和 bestDestructionRate 记录该成员被攻击时对方获得的最佳成绩。
 * @see ClanWarSession
 */
struct ClanWarMember {
    // 成员身份
    std::string memberId;        ///< 成员玩家ID
    std::string memberName;      ///< 成员名称（用于显示）

    // 战斗数据
    std::string mapData;         ///< 成员地图数据（战争开始时快照）

    // 被攻击统计（记录敌方攻击此成员的最佳成绩）
    int bestStars = 0;           ///< 敌方攻击获得的最高星数
    float bestDestructionRate = 0.0f;  ///< 敌方攻击的最高摧毁率

    // 攻击历史
    std::vector<AttackRecord> attacksReceived;  ///< 收到的所有攻击记录
};

/**
 * @struct ClanWarSession
 * @brief 部落战争会话，管理两个部落之间的完整战争流程。
 *
 * 当两个部落匹配成功后创建此会话。会话包含双方部落信息、
 * 所有成员状态、活跃战斗和战争结果统计。
 *
 * 生命周期：
 * 1. 两个部落匹配成功时创建
 * 2. 战争进行中（isActive = true）：成员可发起攻击
 * 3. 战争结束（isActive = false）：统计结果，通知双方
 *
 * @note active_battles 以攻击者ID为键，一个玩家同时只能有一场战斗。
 * @see ClanWarRoom
 * @see ClanWarMember
 */
struct ClanWarSession {
    // 战争标识
    std::string warId;           ///< 战争唯一标识符，格式："WAR_xxx"

    // 参战部落信息
    std::string clan1Id;         ///< 第一个部落的ID
    std::string clan2Id;         ///< 第二个部落的ID
    std::vector<ClanWarMember> clan1Members;  ///< 第一个部落的成员列表
    std::vector<ClanWarMember> clan2Members;  ///< 第二个部落的成员列表

    // 活跃战斗（以攻击者ID为键）
    std::map<std::string, PvpSession> activeBattles;  ///< 当前进行中的战斗

    // 战争统计
    int clan1TotalStars = 0;     ///< 第一个部落获得的总星数
    int clan2TotalStars = 0;     ///< 第二个部落获得的总星数

    // 时间信息
    std::chrono::steady_clock::time_point startTime;  ///< 战争开始时间点
    std::chrono::steady_clock::time_point endTime;    ///< 战争结束时间点

    // 战争状态
    bool isActive = true;        ///< 战争是否进行中
};