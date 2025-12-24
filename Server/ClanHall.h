/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanHall.h
 * File Function: 部落系统管理
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "ClanInfo.h"
#include "PlayerRegistry.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * @class ClanHall
 * @brief 管理部落的创建、加入、离开及信息查询
 */
class ClanHall {
 public:
    /**
     * @brief 构造函数
     * @param registry 玩家注册表指针
     */
    explicit ClanHall(PlayerRegistry* registry);

    /**
     * @brief 创建部落
     * @param player_id 创建者的玩家ID
     * @param clan_name 部落名称
     * @return 创建成功返回true，失败返回false
     */
    bool CreateClan(const std::string& player_id, const std::string& clan_name);

    /**
     * @brief 加入部落
     * @param player_id 玩家ID
     * @param clan_id 部落ID
     * @return 加入成功返回true，失败返回false
     */
    bool JoinClan(const std::string& player_id, const std::string& clan_id);

    /**
     * @brief 离开部落
     * @param player_id 玩家ID
     * @return 离开成功返回true，失败返回false
     */
    bool LeaveClan(const std::string& player_id);

    /**
     * @brief 获取部落列表的JSON表示
     * @return JSON格式的部落列表
     */
    std::string GetClanListJson();

    /**
     * @brief 获取部落成员列表的JSON表示
     * @param clan_id 部落ID
     * @return JSON格式的成员列表
     */
    std::string GetClanMembersJson(const std::string& clan_id);

    /**
     * @brief 检查玩家是否在指定部落中
     * @param player_id 玩家ID
     * @param clan_id 部落ID
     * @return 在部落中返回true，否则返回false
     */
    bool IsPlayerInClan(const std::string& player_id,
                        const std::string& clan_id);

    /**
     * @brief 获取部落的所有成员ID
     * @param clan_id 部落ID
     * @return 成员ID列表
     */
    std::vector<std::string> GetClanMemberIds(const std::string& clan_id);

 private:
    std::map<std::string, ClanInfo> clans_;  // 部落映射表
    std::mutex clan_mutex_;                   // 保护映射表的互斥锁
    PlayerRegistry* player_registry_;         // 玩家注册表指针

    /**
     * @brief 生成唯一的部落ID
     * @return 部落ID字符串
     */
    std::string GenerateClanId();
};