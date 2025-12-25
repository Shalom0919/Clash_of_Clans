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
 * @brief 管理在线玩家的注册、注销和查询。
 *
 * PlayerRegistry 是服务器端管理在线玩家的核心组件，提供线程安全的
 * 玩家上下文存储和检索功能。所有玩家相关的操作都通过此类进行。
 *
 * 主要功能：
 * - 玩家连接时注册玩家上下文
 * - 玩家断开时注销玩家
 * - 通过套接字或玩家ID查找玩家
 * - 获取所有在线玩家的快照
 *
 * 线程安全：
 * 所有公共方法都是线程安全的，内部使用互斥锁保护数据。
 *
 * @note 返回的 PlayerContext 指针在下次 Registry 操作后可能失效，
 *       调用者应在持有返回指针期间避免调用其他 Registry 方法。
 *
 * @see PlayerContext
 */
class PlayerRegistry {
 public:
    /**
     * @brief 注册玩家到注册表中。
     *
     * 将玩家上下文与其套接字关联存储。如果套接字已存在，
     * 则覆盖原有的玩家上下文。
     *
     * @param s 玩家的网络套接字
     * @param ctx 玩家上下文信息
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    void Register(SOCKET s, const PlayerContext& ctx);

    /**
     * @brief 从注册表中注销玩家。
     *
     * 移除与指定套接字关联的玩家上下文。如果套接字不存在，
     * 则不执行任何操作。
     *
     * @param s 要注销的玩家套接字
     *
     * @note 线程安全：此方法内部加锁保护。
     * @note 调用此方法后，之前通过 GetBySocket 或 GetById 返回的
     *       指向该玩家的指针将失效。
     */
    void Unregister(SOCKET s);

    /**
     * @brief 通过套接字获取玩家上下文。
     *
     * @param s 玩家的网络套接字
     * @return 玩家上下文指针，如果不存在则返回 nullptr
     *
     * @warning 返回的指针在下次 Registry 操作（Register/Unregister）后
     *          可能失效。调用者应在短时间内使用完毕。
     * @note 线程安全：此方法内部加锁保护，但返回后锁已释放。
     */
    PlayerContext* GetBySocket(SOCKET s);

    /**
     * @brief 通过玩家ID获取玩家上下文。
     *
     * 遍历所有注册的玩家，查找匹配指定ID的玩家。
     * 时间复杂度为 O(n)，其中 n 为在线玩家数量。
     *
     * @param player_id 要查找的玩家ID
     * @return 玩家上下文指针，如果不存在则返回 nullptr
     *
     * @warning 返回的指针在下次 Registry 操作后可能失效。
     * @note 线程安全：此方法内部加锁保护，但返回后锁已释放。
     */
    PlayerContext* GetById(const std::string& player_id);

    /**
     * @brief 获取所有在线玩家的快照副本。
     *
     * 返回当前所有注册玩家的完整副本。由于返回的是副本，
     * 调用者可以安全地遍历和使用，不受后续 Registry 操作影响。
     *
     * @return 所有玩家的副本映射（套接字 -> 玩家上下文）
     *
     * @note 此方法会复制所有玩家数据，在玩家数量较多时可能有性能开销。
     * @note 线程安全：此方法内部加锁保护。
     */
    std::map<SOCKET, PlayerContext> GetAllSnapshot();

 private:
    std::map<SOCKET, PlayerContext> players_;  ///< 玩家映射表（套接字 -> 上下文）
    std::mutex registry_mutex_;                ///< 保护 players_ 的互斥锁
};