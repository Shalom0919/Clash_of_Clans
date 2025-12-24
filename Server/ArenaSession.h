/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArenaSession.h
 * File Function: PVP竞技场会话管理
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "PlayerRegistry.h"
#include "WarModels.h"

#include <chrono>
#include <map>
#include <mutex>
#include <string>

/**
 * @class ArenaSession
 * @brief 管理PVP战斗会话和观战功能
 */
class ArenaSession {
 public:
    /**
     * @brief 构造函数
     * @param registry 玩家注册表指针
     */
    explicit ArenaSession(PlayerRegistry* registry);

    /**
     * @brief 处理PVP战斗请求
     * @param client_socket 客户端套接字
     * @param target_id 目标玩家ID
     */
    void HandlePvpRequest(SOCKET client_socket, const std::string& target_id);

    /**
     * @brief 处理PVP操作（单位部署）
     * @param client_socket 客户端套接字
     * @param action_data 操作数据，格式："unitType|x|y"
     */
    void HandlePvpAction(SOCKET client_socket, const std::string& action_data);

    /**
     * @brief 处理观战请求
     * @param client_socket 客户端套接字
     * @param target_id 目标玩家ID
     */
    void HandleSpectateRequest(SOCKET client_socket,
                               const std::string& target_id);

    /**
     * @brief 结束PVP会话
     * @param attacker_id 攻击者的玩家ID
     */
    void EndSession(const std::string& attacker_id);

    /**
     * @brief 清理玩家相关的所有会话
     * @param player_id 玩家ID
     */
    void CleanupPlayerSessions(const std::string& player_id);

    /**
     * @brief 获取所有活跃战斗的JSON表示
     * @return JSON格式的战斗状态列表
     */
    std::string GetBattleStatusListJson();

    /**
     * @brief 广播战斗状态给所有在线玩家
     */
    void BroadcastBattleStatusToAll();

 private:
    std::map<std::string, PvpSession> sessions_;  // PVP会话映射（以攻击者ID为键）
    std::mutex session_mutex_;                     // 保护会话的互斥锁
    PlayerRegistry* player_registry_;              // 玩家注册表指针
};