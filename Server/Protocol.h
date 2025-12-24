/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Protocol.h
 * File Function: 网络协议定义
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <cstdint>

// 数据包类型枚举
enum PacketType : uint32_t {
    // 基础功能 (1-9)
    PACKET_LOGIN = 1,
    PACKET_UPLOAD_MAP = 2,
    PACKET_QUERY_MAP = 3,
    PACKET_ATTACK_DATA = 4,
    REQ_USER_LIST = 5,
    RESP_USER_LIST = 6,

    // 匹配系统 (10-19)
    PACKET_FIND_MATCH = 10,
    PACKET_MATCH_FOUND = 11,
    PACKET_MATCH_CANCEL = 12,
    PACKET_ATTACK_START = 13,
    PACKET_ATTACK_RESULT = 14,
    PACKET_BATTLE_REPLAY = 15,

    // 部落系统 (20-29)
    PACKET_CREATE_CLAN = 20,
    PACKET_JOIN_CLAN = 21,
    PACKET_LEAVE_CLAN = 22,
    PACKET_CLAN_LIST = 23,
    PACKET_CLAN_MEMBERS = 24,
    PACKET_CLAN_INFO = 25,

    // 部落战争基础 (30-39)
    PACKET_CLAN_WAR_SEARCH = 30,
    PACKET_CLAN_WAR_MATCH = 31,
    PACKET_CLAN_WAR_ATTACK = 32,
    PACKET_CLAN_WAR_RESULT = 33,
    PACKET_CLAN_WAR_STATUS = 34,
    PACKET_CLAN_WAR_END = 35,  // 部落战争结束通知

    // PVP系统 (40-49)
    PACKET_PVP_REQUEST = 40,
    PACKET_PVP_START = 41,
    PACKET_PVP_ACTION = 42,
    PACKET_PVP_END = 43,
    PACKET_SPECTATE_REQUEST = 44,
    PACKET_SPECTATE_JOIN = 45,

    // 部落战争增强 (50-59)
    PACKET_CLAN_WAR_MEMBER_LIST = 50,
    PACKET_CLAN_WAR_ATTACK_START = 51,
    PACKET_CLAN_WAR_ATTACK_END = 52,
    PACKET_CLAN_WAR_SPECTATE = 53,
    PACKET_CLAN_WAR_STATE_UPDATE = 54,

    // 战斗状态 (60-69)
    PACKET_BATTLE_STATUS_LIST = 60,
    PACKET_BATTLE_STATUS_UPDATE = 61
};

// 数据包头结构
struct PacketHeader {
    uint32_t type;    // 数据包类型
    uint32_t length;  // 数据长度
};