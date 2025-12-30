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
 * @brief 管理部落战争的匹配、攻击、观战和结束流程。
 *
 * ClanWarRoom 是部落战争系统的核心管理类，负责处理从匹配到结束的
 * 完整战争流程。它与 ArenaSession 不同，专门用于部落级别的团队战斗。
 *
 * 主要功能：
 * - 部落匹配：将部落添加到队列，自动匹配实力相近的对手
 * - 战争管理：创建和管理战争会话，跟踪双方成绩
 * - 攻击处理：处理成员发起的攻击，同步战斗状态
 * - 观战支持：允许成员观看队友的战斗
 * - 结果统计：计算星数，确定胜负
 *
 * 部落战争流程：
 * 1. 部落发起战争搜索，加入匹配队列
 * 2. 系统匹配两个部落，创建战争会话
 * 3. 通知双方所有成员战争开始
 * 4. 成员可以攻击敌方成员，获取星数
 * 5. 战争结束时统计总星数，确定胜者
 * 6. 通知所有参与者战争结果
 *
 * 线程安全：
 * - war_mutex_ 保护匹配队列
 * - session_mutex_ 保护活跃战争
 * - 网络发送操作在释放锁后执行，避免死锁
 *
 * @see ClanWarSession
 * @see ClanHall
 * @see PlayerRegistry
 */
class ClanWarRoom {
 public:
    /**
     * @brief 构造函数。
     *
     * @param registry 玩家注册表指针，用于获取玩家信息
     * @param hall 部落大厅指针，用于获取部落成员信息
     *
     * @note 调用者需保证两个指针在 ClanWarRoom 生命周期内有效。
     */
    ClanWarRoom(PlayerRegistry* registry, ClanHall* hall);

    /**
     * @brief 将部落添加到战争匹配队列。
     *
     * 如果部落未在队列中且未参与活跃战争，则将其添加到匹配队列。
     * 队列中有两个或更多部落时，会自动进行匹配。
     *
     * @param clan_id 要加入匹配的部落ID
     *
     * @note 目前匹配算法简单地取前两个部落，未考虑实力匹配。
     * @note 线程安全：此方法内部加锁保护。
     */
    void AddToQueue(const std::string& clan_id);

    /**
     * @brief 处理部落战攻击开始请求。
     *
     * 验证攻击者和目标的状态，如果条件满足则创建战斗会话。
     * 无论成功与否，都会向请求者发送响应。
     *
     * 验证条件：
     * - 攻击者已登录
     * - 战争存在且活跃
     * - 攻击者当前未在战斗中
     * - 目标存在且有地图数据
     *
     * 成功响应格式："{ATTACK}|{targetId}|{mapData}"
     * 失败响应格式："{FAIL}|{reason}|"
     *
     * @param client_socket 攻击者的套接字
     * @param war_id 战争ID
     * @param target_id 目标成员ID
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void HandleAttackStart(SOCKET client_socket,
                           const std::string& war_id,
                           const std::string& target_id);

    /**
     * @brief 处理部落战攻击结束。
     *
     * 记录攻击结果，更新目标成员的被攻击记录和最佳成绩，
     * 累加部落总星数，并广播战争状态更新。
     *
     * @param war_id 战争ID
     * @param record 攻击记录，包含攻击者、星数和摧毁率
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void HandleAttackEnd(const std::string& war_id, const AttackRecord& record);

    /**
     * @brief 处理部落战观战请求。
     *
     * 查找目标玩家在指定战争中参与的活跃战斗，如果找到则将
     * 请求者添加为观战者，并发送战斗信息和历史操作记录。
     *
     * 成功响应格式：
     * "1|{attackerId}|{defenderId}|{mapData}[[[HISTORY]]]{actions}"
     *
     * 失败响应格式：
     * "0|||"
     *
     * @param client_socket 观战请求者的套接字
     * @param war_id 战争ID
     * @param target_id 要观战的玩家ID
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    void HandleSpectate(SOCKET client_socket,
                        const std::string& war_id,
                        const std::string& target_id);

    /**
     * @brief 结束指定的部落战争。
     *
     * 强制结束所有活跃战斗，计算战争结果，通知所有参与者，
     * 并从活跃战争映射中移除会话。
     *
     * 战争结果 JSON 格式：
     * @code
     * {
     *   "warId": "WAR_1",
     *   "clan1Id": "CLAN_1",
     *   "clan2Id": "CLAN_2",
     *   "clan1Stars": 15,
     *   "clan2Stars": 12,
     *   "winnerId": "CLAN_1"  // 平局时为空字符串
     * }
     * @endcode
     *
     * @param war_id 要结束的战争ID
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void EndWar(const std::string& war_id);

    /**
     * @brief 清理玩家相关的所有战争会话。
     *
     * 当玩家断开连接时调用，清理该玩家在所有战争中作为攻击者或
     * 观战者的会话。
     *
     * 清理操作：
     * - 如果玩家是攻击者：结束战斗，通知观战者
     * - 如果玩家是观战者：从观战者列表中移除
     *
     * @param player_id 断开连接的玩家ID
     *
     * @note 线程安全：此方法在锁外发送网络包以避免死锁。
     */
    void CleanupPlayerSessions(const std::string& player_id);

    /**
     * @brief 获取战争成员列表的 JSON 表示。
     *
     * 返回敌方成员列表，包含每个成员的攻击状态和最佳被攻击记录。
     *
     * 返回格式示例：
     * @code
     * {
     *   "warId": "WAR_1",
     *   "clan1TotalStars": 15,
     *   "clan2TotalStars": 12,
     *   "isActive": true,
     *   "enemyMembers": [
     *     {"id": "player1", "name": "玩家1", "bestStars": 2, "bestDestruction": 0.85, "canAttack": true},
     *     ...
     *   ]
     * }
     * @endcode
     *
     * @param war_id 战争ID
     * @param requester_id 请求者的玩家ID（用于确定敌方）
     * @return JSON 格式的成员列表字符串
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    std::string GetMemberListJson(const std::string& war_id,
                                  const std::string& requester_id);

    /**
     * @brief 获取玩家所在的活跃战争ID。
     *
     * 遍历所有活跃战争，查找包含指定玩家的战争。
     *
     * @param player_id 玩家ID
     * @return 战争ID，如果玩家不在任何战争中返回空字符串
     *
     * @note 线程安全：此方法内部加锁保护。
     */
    std::string GetActiveWarIdForPlayer(const std::string& player_id);

 private:
    // 战争数据
    std::map<std::string, ClanWarSession> active_wars_;  ///< 活跃战争（战争ID -> 会话）
    std::vector<std::string> war_queue_;                 ///< 等待匹配的部落队列

    // 同步原语
    std::mutex war_mutex_;                               ///< 保护 war_queue_ 的互斥锁
    std::mutex session_mutex_;                           ///< 保护 active_wars_ 的互斥锁

    // 依赖组件
    PlayerRegistry* player_registry_;                    ///< 玩家注册表（非拥有）
    ClanHall* clan_hall_;                                ///< 部落大厅（非拥有）

    /**
     * @brief 生成唯一的战争ID。
     *
     * 使用静态计数器生成格式为 "WAR_xxx" 的唯一标识符。
     *
     * @return 新生成的战争ID字符串
     */
    std::string GenerateWarId();

    /**
     * @brief 处理匹配队列，尝试匹配两个部落。
     *
     * 当队列中有两个或更多部落时，取出前两个并开始战争。
     *
     * @note 调用时应已持有 war_mutex_。
     */
    void ProcessQueue();

    /**
     * @brief 开始两个部落之间的战争。
     *
     * 创建战争会话，初始化双方成员信息，并通知所有参与者。
     *
     * @param clan1_id 第一个部落ID
     * @param clan2_id 第二个部落ID
     */
    void StartWar(const std::string& clan1_id, const std::string& clan2_id);

    /**
     * @brief 广播战争状态更新给所有参与者。
     *
     * 发送当前星数统计给双方部落的所有成员。
     *
     * @param war_id 战争ID
     */
    void BroadcastWarUpdate(const std::string& war_id);

    /**
     * @brief 广播战争结束通知给所有参与者。
     *
     * @param war_id 战争ID
     * @param result_json 战争结果的 JSON 字符串
     */
    void BroadcastWarEnd(const std::string& war_id, const std::string& result_json);
};