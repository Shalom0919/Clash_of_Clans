/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArenaSession.cpp
 * File Function: PVP 竞技场会话管理实现
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#include "ArenaSession.h"
#include "Protocol.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

extern bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

// ============================================================================
// 协议格式常量
// ============================================================================
namespace {
    constexpr char kFieldSeparator = '|';
    constexpr char kActionSeparator = ',';
    constexpr const char* kHistoryMarker = "[[[HISTORY]]]";
    constexpr const char* kActionDelimiter = "[[[ACTION]]]";
    
    // PVP 响应类型
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
    
    // 战斗结束原因
    constexpr const char* kBattleEnded = "BATTLE_ENDED";
    constexpr const char* kOpponentDisconnected = "OPPONENT_DISCONNECTED";
    constexpr const char* kDefenderDisconnected = "DEFENDER_DISCONNECTED";
}

// ============================================================================
// 构造函数
// ============================================================================

ArenaSession::ArenaSession(PlayerRegistry* registry)
    : player_registry_(registry) {}

// ============================================================================
// PVP 请求处理
// ============================================================================

void ArenaSession::HandlePvpRequest(SOCKET client_socket,
                                    const std::string& target_id) {
    // 获取请求者信息
    PlayerContext* requester = player_registry_->GetBySocket(client_socket);
    if (requester == nullptr) {
        std::string response = std::string(kRoleFail) + kFieldSeparator + 
                               kReasonNotLoggedIn + kFieldSeparator;
        sendPacket(client_socket, PACKET_PVP_START, response);
        return;
    }

    std::string requester_id = requester->playerId;

    // 验证：不能攻击自己
    if (requester_id == target_id) {
        std::string response = std::string(kRoleFail) + kFieldSeparator + 
                               kReasonCannotAttackSelf + kFieldSeparator;
        sendPacket(client_socket, PACKET_PVP_START, response);
        return;
    }

    // 获取目标玩家信息
    PlayerContext* target = player_registry_->GetById(target_id);
    if (target == nullptr) {
        std::string response = std::string(kRoleFail) + kFieldSeparator + 
                               kReasonTargetOffline + kFieldSeparator;
        sendPacket(client_socket, PACKET_PVP_START, response);
        return;
    }

    // 验证：目标必须有地图数据
    std::string target_map_data = target->mapData;
    if (target_map_data.empty()) {
        std::string response = std::string(kRoleFail) + kFieldSeparator + 
                               kReasonNoMap + kFieldSeparator;
        sendPacket(client_socket, PACKET_PVP_START, response);
        return;
    }

    SOCKET target_socket = target->socket;

    // 创建会话（需要加锁）
    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        // 验证：请求者不能已在战斗中
        if (sessions_.find(requester_id) != sessions_.end()) {
            std::string response = std::string(kRoleFail) + kFieldSeparator + 
                                   kReasonAlreadyInBattle + kFieldSeparator;
            sendPacket(client_socket, PACKET_PVP_START, response);
            return;
        }

        // 验证：目标不能已在战斗中
        for (const auto& pair : sessions_) {
            if (pair.second.isActive &&
                (pair.second.defenderId == target_id ||
                 pair.second.attackerId == target_id)) {
                std::string response = std::string(kRoleFail) + kFieldSeparator + 
                                       kReasonTargetInBattle + kFieldSeparator;
                sendPacket(client_socket, PACKET_PVP_START, response);
                return;
            }
        }

        // 创建新会话
        PvpSession session;
        session.attackerId = requester_id;
        session.defenderId = target_id;
        session.mapData = target_map_data;
        session.isActive = true;
        session.startTime = std::chrono::steady_clock::now();
        session.actionHistory.clear();

        sessions_[requester_id] = session;

        std::cout << "[PVP] 会话创建: " << requester_id << " vs " << target_id
                  << std::endl;
    }

    // 发送响应（在锁外进行网络操作，避免死锁）
    std::string attacker_msg = std::string(kRoleAttack) + kFieldSeparator + 
                               target_id + kFieldSeparator + target_map_data;
    sendPacket(client_socket, PACKET_PVP_START, attacker_msg);

    std::string defender_msg = std::string(kRoleDefend) + kFieldSeparator + 
                               requester_id + kFieldSeparator;
    sendPacket(target_socket, PACKET_PVP_START, defender_msg);

    // 广播战斗状态更新
    BroadcastBattleStatusToAll();
}

// ============================================================================
// PVP 操作处理
// ============================================================================

void ArenaSession::HandlePvpAction(SOCKET client_socket,
                                   const std::string& action_data) {
    PlayerContext* player = player_registry_->GetBySocket(client_socket);
    if (player == nullptr) {
        return;
    }

    std::string player_id = player->playerId;
    
    // 收集需要通知的目标（在锁内获取信息，锁外发送）
    std::string defender_id;
    SOCKET defender_socket = INVALID_SOCKET;
    std::vector<std::pair<std::string, SOCKET>> spectators_to_notify;
    bool session_found = false;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        
        auto it = sessions_.find(player_id);
        if (it != sessions_.end() && it->second.isActive) {
            // 记录操作历史
            it->second.actionHistory.push_back(action_data);
            
            defender_id = it->second.defenderId;
            session_found = true;

            std::cout << "[PVP] 操作记录: " << player_id
                      << " - " << action_data
                      << " (历史: " << it->second.actionHistory.size() << ")"
                      << std::endl;

            // 获取防守者 socket
            PlayerContext* defender = player_registry_->GetById(defender_id);
            if (defender != nullptr && defender->socket != INVALID_SOCKET) {
                defender_socket = defender->socket;
            }

            // 收集观战者 socket
            for (const auto& spectator_id : it->second.spectatorIds) {
                PlayerContext* spectator = player_registry_->GetById(spectator_id);
                if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                    spectators_to_notify.push_back({spectator_id, spectator->socket});
                }
            }
        }
    }

    if (!session_found) {
        std::cout << "[PVP] 警告: 玩家 " << player_id 
                  << " 没有活跃会话，忽略操作" << std::endl;
        return;
    }

    // 在锁外发送网络包（避免死锁）
    if (defender_socket != INVALID_SOCKET) {
        sendPacket(defender_socket, PACKET_PVP_ACTION, action_data);
    }

    for (const auto& pair : spectators_to_notify) {
        sendPacket(pair.second, PACKET_PVP_ACTION, action_data);
    }
}

// ============================================================================
// 观战请求处理
// ============================================================================

void ArenaSession::HandleSpectateRequest(SOCKET client_socket,
                                         const std::string& target_id) {
    PlayerContext* requester = player_registry_->GetBySocket(client_socket);
    if (requester == nullptr) {
        sendPacket(client_socket, PACKET_SPECTATE_JOIN, "0|||0|");
        return;
    }

    std::string spectator_id = requester->playerId;
    
    // 用于存储观战信息
    std::string attacker_id, defender_id, map_data;
    std::vector<std::string> history;
    int64_t elapsed_ms = 0;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        for (auto& pair : sessions_) {
            if (!pair.second.isActive) {
                continue;
            }

            // 检查目标是否为攻击者或防守者
            if (pair.second.attackerId == target_id ||
                pair.second.defenderId == target_id) {
                
                attacker_id = pair.second.attackerId;
                defender_id = pair.second.defenderId;
                map_data = pair.second.mapData;
                history = pair.second.actionHistory;

                // 计算已进行时间
                auto now = std::chrono::steady_clock::now();
                elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - pair.second.startTime).count();

                // 添加观战者（防止重复）
                auto& spectators = pair.second.spectatorIds;
                if (std::find(spectators.begin(), spectators.end(), spectator_id) 
                    == spectators.end()) {
                    spectators.push_back(spectator_id);
                }
                
                found = true;

                std::cout << "[Spectate] " << spectator_id << " 正在观看 "
                          << attacker_id << " vs " << defender_id
                          << " (已进行: " << elapsed_ms << "ms, 历史操作: "
                          << history.size() << ")" << std::endl;
                break;
            }
        }
    }

    if (!found || map_data.empty()) {
        std::cout << "[Spectate] 观战请求失败: 目标 " << target_id 
                  << " 没有活跃战斗" << std::endl;
        sendPacket(client_socket, PACKET_SPECTATE_JOIN, "0|||0|");
        return;
    }

    // 构建响应
    // 格式: "1|attackerId|defenderId|elapsedMs|mapData[[[HISTORY]]]action1[[[ACTION]]]action2..."
    std::ostringstream oss;
    oss << "1" << kFieldSeparator
        << attacker_id << kFieldSeparator
        << defender_id << kFieldSeparator
        << elapsed_ms << kFieldSeparator
        << map_data;

    if (!history.empty()) {
        oss << kHistoryMarker;
        for (size_t i = 0; i < history.size(); ++i) {
            if (i > 0) {
                oss << kActionDelimiter;
            }
            oss << history[i];
        }
    }

    sendPacket(client_socket, PACKET_SPECTATE_JOIN, oss.str());
}

// ============================================================================
// 结束会话
// ============================================================================

void ArenaSession::EndSession(const std::string& attacker_id) {
    // 收集需要通知的目标
    std::string defender_id;
    SOCKET defender_socket = INVALID_SOCKET;
    std::vector<std::pair<std::string, SOCKET>> spectators_to_notify;
    bool session_found = false;
    size_t total_action_count = 0;  // 🔧 新增：总操作数量

    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        
        auto it = sessions_.find(attacker_id);
        if (it == sessions_.end()) {
            std::cout << "[PVP] EndSession: 会话 " << attacker_id 
                      << " 不存在" << std::endl;
            return;
        }

        // 标记为非活跃
        it->second.isActive = false;
        defender_id = it->second.defenderId;
        total_action_count = it->second.actionHistory.size();  // 🔧 获取总操作数

        // 收集防守者 socket
        PlayerContext* defender = player_registry_->GetById(defender_id);
        if (defender != nullptr && defender->socket != INVALID_SOCKET) {
            defender_socket = defender->socket;
        }

        // 收集观战者 socket
        for (const auto& spectator_id : it->second.spectatorIds) {
            PlayerContext* spectator = player_registry_->GetById(spectator_id);
            if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                spectators_to_notify.push_back({spectator_id, spectator->socket});
            }
        }

        sessions_.erase(it);
        session_found = true;

        std::cout << "[PVP] 会话结束: " << attacker_id
                  << " (防守方: " << defender_id
                  << ", 观战者: " << spectators_to_notify.size() << "人"
                  << ", 总操作数: " << total_action_count << ")"
                  << std::endl;
    }

    if (!session_found) {
        return;
    }

    // 🔧 修复：构建包含总操作数的结束消息
    // 格式: "BATTLE_ENDED|totalActionCount"
    std::ostringstream end_msg;
    end_msg << kBattleEnded << kFieldSeparator << total_action_count;
    std::string end_message = end_msg.str();

    // 在锁外发送网络包
    if (defender_socket != INVALID_SOCKET) {
        sendPacket(defender_socket, PACKET_PVP_END, end_message);
        std::cout << "[PVP] 已通知防守方: " << defender_id << std::endl;
    }

    for (const auto& pair : spectators_to_notify) {
        sendPacket(pair.second, PACKET_PVP_END, end_message);
        std::cout << "[PVP] 已通知观战者: " << pair.first 
                  << " (总操作数: " << total_action_count << ")" << std::endl;
    }

    BroadcastBattleStatusToAll();
}

// ============================================================================
// 玩家断开连接清理
// ============================================================================

void ArenaSession::CleanupPlayerSessions(const std::string& player_id) {
    // 收集需要通知的目标
    struct NotifyTarget {
        std::string id;
        SOCKET socket;
        size_t action_count;
    };
    std::vector<NotifyTarget> defenders_to_notify;
    std::vector<NotifyTarget> attackers_to_notify;
    std::vector<NotifyTarget> spectators_to_notify;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        // 清理玩家作为攻击者的会话
        auto it = sessions_.find(player_id);
        if (it != sessions_.end()) {
            std::cout << "[PVP] 清理攻击者会话: " << player_id << std::endl;

            PvpSession& session = it->second;
            session.isActive = false;
            size_t action_count = session.actionHistory.size();

            // 收集防守方
            PlayerContext* defender = player_registry_->GetById(session.defenderId);
            if (defender != nullptr && defender->socket != INVALID_SOCKET) {
                defenders_to_notify.push_back({session.defenderId, defender->socket, action_count});
            }

            // 收集观战者
            for (const auto& spectator_id : session.spectatorIds) {
                PlayerContext* spectator = player_registry_->GetById(spectator_id);
                if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                    spectators_to_notify.push_back({spectator_id, spectator->socket, action_count});
                }
            }

            sessions_.erase(it);
        }

        // 清理玩家作为防守者或观战者的会话
        for (auto session_it = sessions_.begin(); session_it != sessions_.end();) {
            auto& session = session_it->second;

            // 从观战者列表中移除
            auto spectator_it = std::find(session.spectatorIds.begin(),
                                          session.spectatorIds.end(), 
                                          player_id);
            if (spectator_it != session.spectatorIds.end()) {
                session.spectatorIds.erase(spectator_it);
                std::cout << "[PVP] 从会话中移除观战者: " << player_id << std::endl;
            }

            // 如果防守者断开连接，结束会话
            if (session.defenderId == player_id) {
                std::cout << "[PVP] 防守者断开连接，结束会话: " 
                          << session_it->first << std::endl;

                session.isActive = false;
                size_t action_count = session.actionHistory.size();

                // 收集攻击方
                PlayerContext* attacker = player_registry_->GetById(session.attackerId);
                if (attacker != nullptr && attacker->socket != INVALID_SOCKET) {
                    attackers_to_notify.push_back({session.attackerId, attacker->socket, action_count});
                }

                // 收集观战者
                for (const auto& spectator_id : session.spectatorIds) {
                    PlayerContext* spectator = player_registry_->GetById(spectator_id);
                    if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                        spectators_to_notify.push_back({spectator_id, spectator->socket, action_count});
                    }
                }

                session_it = sessions_.erase(session_it);
            } else {
                ++session_it;
            }
        }
    }

    // 在锁外发送网络包
    // 🔧 修复：发送包含总操作数的结束消息
    for (const auto& target : defenders_to_notify) {
        std::ostringstream oss;
        oss << kOpponentDisconnected << kFieldSeparator << target.action_count;
        sendPacket(target.socket, PACKET_PVP_END, oss.str());
        std::cout << "[PVP] 通知防守方攻击者断开: " << target.id << std::endl;
    }

    for (const auto& target : attackers_to_notify) {
        std::ostringstream oss;
        oss << kDefenderDisconnected << kFieldSeparator << target.action_count;
        sendPacket(target.socket, PACKET_PVP_END, oss.str());
        std::cout << "[PVP] 通知攻击方防守者断开: " << target.id << std::endl;
    }

    for (const auto& target : spectators_to_notify) {
        std::ostringstream oss;
        oss << kBattleEnded << kFieldSeparator << target.action_count;
        sendPacket(target.socket, PACKET_PVP_END, oss.str());
        std::cout << "[PVP] 通知观战者战斗结束: " << target.id 
                  << " (总操作数: " << target.action_count << ")" << std::endl;
    }

    BroadcastBattleStatusToAll();
}

// ============================================================================
// 战斗状态获取与广播
// ============================================================================

std::string ArenaSession::GetBattleStatusListJson() {
    std::lock_guard<std::mutex> lock(session_mutex_);

    std::ostringstream oss;
    oss << "{\"statuses\":[";

    bool first = true;
    for (const auto& pair : sessions_) {
        const PvpSession& session = pair.second;
        if (!session.isActive) {
            continue;
        }

        // 攻击者状态
        if (!first) {
            oss << ",";
        }
        first = false;

        oss << "{\"userId\":\"" << session.attackerId << "\","
            << "\"inBattle\":true,"
            << "\"opponentId\":\"" << session.defenderId << "\","
            << "\"isAttacker\":true}";

        // 防守者状态
        oss << ",{\"userId\":\"" << session.defenderId << "\","
            << "\"inBattle\":true,"
            << "\"opponentId\":\"" << session.attackerId << "\","
            << "\"isAttacker\":false}";
    }

    oss << "]}";
    return oss.str();
}

void ArenaSession::BroadcastBattleStatusToAll() {
    std::string status_json = GetBattleStatusListJson();

    auto all_players = player_registry_->GetAllSnapshot();
    for (const auto& pair : all_players) {
        if (!pair.second.playerId.empty()) {
            sendPacket(pair.first, PACKET_BATTLE_STATUS_LIST, status_json);
        }
    }
}