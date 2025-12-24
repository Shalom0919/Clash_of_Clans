/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanWarRoom.h
 * File Function: 部落战争系统管理
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "ClanHall.h"
#include "PlayerRegistry.h"
#include "WarModels.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * @class ClanWarRoom
 * @brief 管理部落战争的匹配、攻击、观战和结束流程
 */
class ClanWarRoom {
 public:
    /**
     * @brief 构造函数
     * @param registry 玩家注册表指针
     * @param hall 部落大厅指针
     */
    ClanWarRoom(PlayerRegistry* registry, ClanHall* hall);

    /**
     * @brief 将部落添加到战争匹配队列
     * @param clan_id 部落ID
     */
    void AddToQueue(const std::string& clan_id);

    /**
     * @brief 处理攻击开始请求
     * @param client_socket 客户端套接字
     * @param war_id 战争ID
     * @param target_id 目标玩家ID
     */
    void HandleAttackStart(SOCKET client_socket,
                           const std::string& war_id,
                           const std::string& target_id);

    /**
     * @brief 处理攻击结束
     * @param war_id 战争ID
     * @param record 攻击记录
     */
    void HandleAttackEnd(const std::string& war_id, const AttackRecord& record);

    /**
     * @brief 处理观战请求
     * @param client_socket 客户端套接字
     * @param war_id 战争ID
     * @param target_id 目标玩家ID
     */
    void HandleSpectate(SOCKET client_socket,
                        const std::string& war_id,
                        const std::string& target_id);

    /**
     * @brief 结束指定的部落战争
     * @param war_id 战争ID
     */
    void EndWar(const std::string& war_id);

    /**
     * @brief 清理玩家相关的所有战争会话
     * @param player_id 玩家ID
     */
    void CleanupPlayerSessions(const std::string& player_id);

    /**
     * @brief 获取战争成员列表的JSON表示
     * @param war_id 战争ID
     * @param requester_id 请求者的玩家ID
     * @return JSON格式的成员列表
     */
    std::string GetMemberListJson(const std::string& war_id,
                                  const std::string& requester_id);

    /**
     * @brief 获取玩家所在的活跃战争ID
     * @param player_id 玩家ID
     * @return 战争ID，如果不在战争中返回空字符串
     */
    std::string GetActiveWarIdForPlayer(const std::string& player_id);

 private:
    std::map<std::string, ClanWarSession> active_wars_;  // 活跃的部落战争
    std::vector<std::string> war_queue_;                 // 等待匹配的部落队列
    std::mutex war_mutex_;                               // 保护队列的互斥锁
    std::mutex session_mutex_;                           // 保护会话的互斥锁

    PlayerRegistry* player_registry_;  // 玩家注册表
    ClanHall* clan_hall_;              // 部落大厅

    /**
     * @brief 生成唯一的战争ID
     * @return 战争ID字符串
     */
    std::string GenerateWarId();

    /**
     * @brief 处理匹配队列，尝试匹配两个部落
     */
    void ProcessQueue();

    /**
     * @brief 开始两个部落之间的战争
     * @param clan1_id 第一个部落ID
     * @param clan2_id 第二个部落ID
     */
    void StartWar(const std::string& clan1_id, const std::string& clan2_id);

    /**
     * @brief 广播战争状态更新给所有参与者
     * @param war_id 战争ID
     */
    void BroadcastWarUpdate(const std::string& war_id);

    /**
     * @brief 广播战争结束通知给所有参与者
     * @param war_id 战争ID
     * @param result_json 战争结果的JSON字符串
     */
    void BroadcastWarEnd(const std::string& war_id, const std::string& result_json);
};