/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PlayerRegistry.h
 * File Function: 玩家注册管理
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "ClanInfo.h"

#include <WinSock2.h>

#include <map>
#include <mutex>
#include <string>

/**
 * @class PlayerRegistry
 * @brief 管理在线玩家的注册、注销和查询
 */
class PlayerRegistry {
 public:
    /**
     * @brief 注册玩家
     * @param s 玩家的套接字
     * @param ctx 玩家上下文
     */
    void Register(SOCKET s, const PlayerContext& ctx);

    /**
     * @brief 注销玩家
     * @param s 玩家的套接字
     */
    void Unregister(SOCKET s);

    /**
     * @brief 通过套接字获取玩家
     * @param s 套接字
     * @return 玩家上下文指针，不存在返回nullptr
     * @note 返回的指针在下次Registry操作后可能失效
     */
    PlayerContext* GetBySocket(SOCKET s);

    /**
     * @brief 通过玩家ID获取玩家
     * @param player_id 玩家ID
     * @return 玩家上下文指针，不存在返回nullptr
     * @note 返回的指针在下次Registry操作后可能失效
     */
    PlayerContext* GetById(const std::string& player_id);

    /**
     * @brief 获取所有玩家的快照副本
     * @return 所有玩家的副本映射
     */
    std::map<SOCKET, PlayerContext> GetAllSnapshot();

 private:
    std::map<SOCKET, PlayerContext> players_;  // 玩家映射表
    std::mutex registry_mutex_;                // 保护映射表的互斥锁
};