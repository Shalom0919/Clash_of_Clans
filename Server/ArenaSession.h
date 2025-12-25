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
 * @brief 管理 PVP 战斗会话和观战功能。
 *
 * ArenaSession 负责处理玩家之间的实时 PVP 战斗，包括发起战斗、
 * 同步操作、观战加入和会话结束。它是独立于部落战争的 PVP 系统。
 *
 * 主要功能：
 * - 处理 PVP 战斗请求，验证双方状态
 * - 同步攻击者的操作到防守方和观战者
 * - 管理观战者加入和历史操作回放
 * - 处理玩家断开连接时的会话清理
 * - 广播战斗状态给所有在线玩家
 *
 * PVP 战斗流程：
 * 1. 攻击者发送 PVP 请求，指定目标玩家
 * 2. 服务器验证双方状态，创建会话
 * 3. 通知双方战斗开始，发送地图数据给攻击者
 * 4. 攻击者的每个操作同步到防守方和观战者
 * 5. 战斗结束时清理会话，通知所有参与者
 *
 * 线程安全：
 * 所有公共方法都是线程安全的。为避免死锁，网络发送操作
 * 在释放互斥锁后执行。
 *
 * @note 会话以攻击者ID为键存储，一个玩家同时只能发起一场战斗。
 *
 * @see PvpSession
 * @see PlayerRegistry
 */
class ArenaSession {
 public:
    /**
     * @brief 构造函数。
     *
     * @param registry 玩家注册表指针，用于获取玩家信息和发送通知。
     *                 调用者需保证 registry 在 ArenaSession 生命周期内有效。
     */
    explicit ArenaSession(PlayerRegistry* registry);

    /**
     * @brief 处理 PVP 战斗请求。
     *
     * 验证请求者和目标的状态，如果条件满足则创建 PVP 会话。
     * 无论成功与否，都会向请求者发送响应。
     *
     * 验证条件：
     * - 请求者已登录
     * - 不能攻击自己
     * - 目标玩家在线
     * - 目标玩家有地图数据
     * - 请求者当前未在战斗中
     * - 目标当前未在战斗中
     *
     * 成功响应格式："{ATTACK}|{targetId}|{mapData}"
     * 失败响应格式："{FAIL}|{reason}|"
     *
     * @param client_socket 请求者的套接字
     * @param target_id 目标玩家ID
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void HandlePvpRequest(SOCKET client_socket, const std::string& target_id);

    /**
     * @brief 处理 PVP 操作（单位部署）。
     *
     * 记录攻击者的操作到历史记录，并同步到防守方和所有观战者。
     * 如果发送者不是活跃战斗的攻击者，操作将被忽略。
     *
     * 操作数据格式："{elapsedMs},{unitType},{x},{y}"
     *
     * @param client_socket 发送操作的客户端套接字
     * @param action_data 操作数据
     *
     * @note 操作历史用于观战者加入时回放已发生的操作。
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void HandlePvpAction(SOCKET client_socket, const std::string& action_data);

    /**
     * @brief 处理观战请求。
     *
     * 查找目标玩家参与的活跃战斗，如果找到则将请求者添加为观战者，
     * 并发送战斗信息和历史操作记录。
     *
     * 成功响应格式：
     * "1|{attackerId}|{defenderId}|{elapsedMs}|{mapData}[[[HISTORY]]]{actions}"
     *
     * 失败响应格式：
     * "0|||0|"
     *
     * @param client_socket 观战请求者的套接字
     * @param target_id 要观战的玩家ID（可以是攻击者或防守者）
     *
     * @note 观战者会收到历史操作记录用于追赶进度。
     * @note 线程安全：此方法内部加锁保护。
     */
    void HandleSpectateRequest(SOCKET client_socket,
                               const std::string& target_id);

    /**
     * @brief 结束指定攻击者的 PVP 会话。
     *
     * 标记会话为非活跃，通知防守方和所有观战者战斗结束，
     * 然后从会话映射中移除会话。
     *
     * 结束消息格式："{BATTLE_ENDED}|{totalActionCount}"
     *
     * @param attacker_id 攻击者的玩家ID（会话的键）
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void EndSession(const std::string& attacker_id);

    /**
     * @brief 清理指定玩家相关的所有会话。
     *
     * 当玩家断开连接时调用，清理该玩家作为攻击者、防守者或
     * 观战者参与的所有会话。
     *
     * 清理操作：
     * - 如果玩家是攻击者：结束会话，通知防守方和观战者
     * - 如果玩家是防守者：结束会话，通知攻击者和观战者
     * - 如果玩家是观战者：从观战者列表中移除
     *
     * @param player_id 断开连接的玩家ID
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void CleanupPlayerSessions(const std::string& player_id);

    /**
     * @brief 获取所有活跃战斗的 JSON 状态列表。
     *
     * 返回格式示例：
     * @code
     * {
     *   "statuses": [
     *     {"userId": "player1", "inBattle": true, "opponentId": "player2", "isAttacker": true},
     *     {"userId": "player2", "inBattle": true, "opponentId": "player1", "isAttacker": false}
     *   ]
     * }
     * @endcode
     *
     * @return JSON 格式的战斗状态列表字符串
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    std::string GetBattleStatusListJson();

    /**
     * @brief 广播战斗状态给所有在线玩家。
     *
     * 获取当前所有活跃战斗的状态，并发送给每个已登录的在线玩家。
     * 用于更新客户端的在线玩家列表中的战斗状态标记。
     *
     * @note 此方法会遍历所有在线玩家，在玩家数量较多时可能有性能开销。
     */
    void BroadcastBattleStatusToAll();

 private:
    std::map<std::string, PvpSession> sessions_;  ///< PVP 会话映射（攻击者ID -> 会话）
    std::mutex session_mutex_;                     ///< 保护 sessions_ 的互斥锁
    PlayerRegistry* player_registry_;              ///< 玩家注册表指针（非拥有）
};