/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Protocol.h
 * File Function: 网络协议定义（客户端与服务器共享）
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <cstdint>
#include <string>

// ============================================================================
// 数据包类型定义
// ============================================================================
// 命名规范：PACKET_<模块>_<动作>
// 数值分配：每个模块占用10个数值空间

enum PacketType : uint32_t {
    // ======================== 基础功能 (1-9) ========================
    PACKET_LOGIN = 1,           // 登录请求/响应
    PACKET_UPLOAD_MAP = 2,      // 上传地图数据
    PACKET_QUERY_MAP = 3,       // 查询地图数据
    PACKET_ATTACK_DATA = 4,     // 攻击数据（已废弃）
    PACKET_USER_LIST_REQ = 5,   // 请求在线用户列表
    PACKET_USER_LIST_RESP = 6,  // 响应在线用户列表

    // ======================== 匹配系统 (10-19) ========================
    PACKET_MATCH_FIND = 10,     // 发起匹配请求
    PACKET_MATCH_FOUND = 11,    // 匹配成功通知
    PACKET_MATCH_CANCEL = 12,   // 取消匹配
    PACKET_ATTACK_START = 13,   // 开始攻击
    PACKET_ATTACK_RESULT = 14,  // 攻击结果
    PACKET_BATTLE_REPLAY = 15,  // 战斗回放数据

    // ======================== 部落系统 (20-29) ========================
    PACKET_CLAN_CREATE = 20,    // 创建部落
    PACKET_CLAN_JOIN = 21,      // 加入部落
    PACKET_CLAN_LEAVE = 22,     // 离开部落
    PACKET_CLAN_LIST = 23,      // 获取部落列表
    PACKET_CLAN_MEMBERS = 24,   // 获取部落成员
    PACKET_CLAN_INFO = 25,      // 获取部落信息

    // ======================== 部落战争基础 (30-39) ========================
    PACKET_WAR_SEARCH = 30,       // 搜索部落战
    PACKET_WAR_MATCH = 31,        // 部落战匹配成功
    PACKET_WAR_ATTACK = 32,       // 部落战攻击
    PACKET_WAR_RESULT = 33,       // 部落战攻击结果
    PACKET_WAR_STATUS = 34,       // 部落战状态更新
    PACKET_WAR_END = 35,          // 部落战结束

    // ======================== PVP 实时对战 (40-49) ========================
    PACKET_PVP_REQUEST = 40,      // 发起 PVP 请求
    PACKET_PVP_START = 41,        // PVP 开始通知
    PACKET_PVP_ACTION = 42,       // PVP 操作同步（单位部署）
    PACKET_PVP_END = 43,          // PVP 结束通知
    PACKET_SPECTATE_REQUEST = 44, // 观战请求
    PACKET_SPECTATE_JOIN = 45,    // 观战加入响应

    // ======================== 部落战争增强 (50-59) ========================
    PACKET_WAR_MEMBER_LIST = 50,    // 部落战成员列表
    PACKET_WAR_ATTACK_START = 51,   // 部落战攻击开始
    PACKET_WAR_ATTACK_END = 52,     // 部落战攻击结束
    PACKET_WAR_SPECTATE = 53,       // 部落战观战
    PACKET_WAR_STATE_UPDATE = 54,   // 部落战状态更新

    // ======================== 战斗状态广播 (60-69) ========================
    PACKET_BATTLE_STATUS_LIST = 60,   // 战斗状态列表
    PACKET_BATTLE_STATUS_UPDATE = 61  // 战斗状态更新
};

// ============================================================================
// 数据包头结构
// ============================================================================

struct PacketHeader {
    uint32_t type;    // 数据包类型（PacketType 枚举值）
    uint32_t length;  // 数据体长度（不含包头）
};

// ============================================================================
// 协议数据格式常量
// ============================================================================

namespace ProtocolFormat {
    // 字段分隔符
    constexpr char kFieldSeparator = '|';
    
    // 历史记录标记（用于观战数据）
    constexpr const char* kHistoryMarker = "[[[HISTORY]]]";
    
    // 操作分隔符（用于观战历史中的多个操作）
    constexpr const char* kActionSeparator = "[[[ACTION]]]";
    
    // PVP 操作数据分隔符（更简洁的逗号分隔）
    constexpr char kActionFieldSeparator = ',';
}

// ============================================================================
// PVP 响应类型常量
// ============================================================================

namespace PvpResponse {
    // 角色类型
    constexpr const char* kRoleAttack = "ATTACK";
    constexpr const char* kRoleDefend = "DEFEND";
    constexpr const char* kRoleFail = "FAIL";
    
    // 失败原因
    constexpr const char* kReasonNotLoggedIn = "NOT_LOGGED_IN";
    constexpr const char* kReasonTargetOffline = "TARGET_OFFLINE";
    constexpr const char* kReasonNoMap = "NO_MAP";
    constexpr const char* kReasonAlreadyInBattle = "ALREADY_IN_BATTLE";
    constexpr const char* kReasonTargetInBattle = "TARGET_IN_BATTLE";
    constexpr const char* kReasonCannotAttackSelf = "CANNOT_ATTACK_SELF";
}

// ============================================================================
// 战斗结束原因
// ============================================================================

namespace BattleEndReason {
    constexpr const char* kBattleEnded = "BATTLE_ENDED";
    constexpr const char* kOpponentDisconnected = "OPPONENT_DISCONNECTED";
    constexpr const char* kDefenderDisconnected = "DEFENDER_DISCONNECTED";
    constexpr const char* kWarEnded = "WAR_ENDED";
}