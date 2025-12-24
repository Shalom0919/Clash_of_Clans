/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArenaSession.cpp
 * File Function: PVP竞技场会话管理实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ArenaSession.h"

#include "Protocol.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

extern bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

ArenaSession::ArenaSession(PlayerRegistry* registry)
    : player_registry_(registry) {}

void ArenaSession::HandlePvpRequest(SOCKET client_socket,
                                    const std::string& target_id) {
    PlayerContext* requester = player_registry_->GetBySocket(client_socket);
    if (requester == nullptr) {
        sendPacket(client_socket, PACKET_PVP_START, "FAIL|NOT_LOGGED_IN|");
        return;
    }

    std::string requester_id = requester->playerId;

    // 不允许攻击自己
    if (requester_id == target_id) {
        sendPacket(client_socket, PACKET_PVP_START, "FAIL|CANNOT_ATTACK_SELF|");
        return;
    }

    PlayerContext* target = player_registry_->GetById(target_id);
    if (target == nullptr) {
        sendPacket(client_socket, PACKET_PVP_START, "FAIL|TARGET_OFFLINE|");
        return;
    }

    std::string target_map_data = target->mapData;
    if (target_map_data.empty()) {
        sendPacket(client_socket, PACKET_PVP_START, "FAIL|NO_MAP|");
        return;
    }

    SOCKET target_socket = target->socket;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        // 检查是否已在战斗中
        if (sessions_.find(requester_id) != sessions_.end()) {
            sendPacket(client_socket, PACKET_PVP_START,
                       "FAIL|ALREADY_IN_BATTLE|");
            return;
        }

        // 检查目标是否正在被攻击
        for (const auto& pair : sessions_) {
            if (pair.second.isActive && 
                (pair.second.defenderId == target_id || 
                 pair.second.attackerId == target_id)) {
                sendPacket(client_socket, PACKET_PVP_START,
                           "FAIL|TARGET_IN_BATTLE|");
                return;
            }
        }

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

    // 发送响应（在锁外进行网络操作）
    std::string attacker_msg = "ATTACK|" + target_id + "|" + target_map_data;
    sendPacket(client_socket, PACKET_PVP_START, attacker_msg);

    std::string defender_msg = "DEFEND|" + requester_id + "|";
    sendPacket(target_socket, PACKET_PVP_START, defender_msg);

    BroadcastBattleStatusToAll();
}

void ArenaSession::HandlePvpAction(SOCKET client_socket,
                                   const std::string& action_data) {
    PlayerContext* player = player_registry_->GetBySocket(client_socket);
    if (player == nullptr) {
        return;
    }

    std::string player_id = player->playerId;
    PvpSession session_copy;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        auto it = sessions_.find(player_id);
        if (it != sessions_.end() && it->second.isActive) {
            // 记录操作历史（统一格式："unitType,x,y"）
            it->second.actionHistory.push_back(action_data);

            session_copy = it->second;
            found = true;
            
            std::cout << "[PVP] 操作记录: " << player_id 
                      << " - " << action_data 
                      << " (历史: " << it->second.actionHistory.size() << ")"
                      << std::endl;
        }
    }

    if (!found) {
        std::cout << "[PVP] 警告: 玩家 " << player_id << " 没有活跃会话，忽略操作" << std::endl;
        return;
    }

    // 广播给防守方和观战者（在锁外进行网络操作）
    PlayerContext* defender =
        player_registry_->GetById(session_copy.defenderId);
    if (defender != nullptr && defender->socket != INVALID_SOCKET) {
        sendPacket(defender->socket, PACKET_PVP_ACTION, action_data);
    }

    for (const auto& spectator_id : session_copy.spectatorIds) {
        PlayerContext* spectator = player_registry_->GetById(spectator_id);
        if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
            sendPacket(spectator->socket, PACKET_PVP_ACTION, action_data);
        }
    }
}

void ArenaSession::HandleSpectateRequest(SOCKET client_socket,
                                         const std::string& target_id) {
    PlayerContext* requester = player_registry_->GetBySocket(client_socket);
    if (requester == nullptr) {
        sendPacket(client_socket, PACKET_SPECTATE_JOIN, "0|||0|");
        return;
    }

    std::string spectator_id = requester->playerId;
    std::string attacker_id, defender_id, map_data;
    std::vector<std::string> history;
    int64_t elapsed_ms = 0;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        for (auto& pair : sessions_) {
            // 检查会话是否活跃
            if (!pair.second.isActive) {
                continue;
            }

            if (pair.second.attackerId == target_id ||
                pair.second.defenderId == target_id) {
                attacker_id = pair.second.attackerId;
                defender_id = pair.second.defenderId;
                map_data = pair.second.mapData;
                history = pair.second.actionHistory;

                // 计算战斗已经进行的时间
                auto now = std::chrono::steady_clock::now();
                elapsed_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - pair.second.startTime)
                        .count();

                // 防止重复添加观战者
                auto& spectators = pair.second.spectatorIds;
                if (std::find(spectators.begin(), spectators.end(), spectator_id) == spectators.end()) {
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
        std::cout << "[Spectate] 观战请求失败: 目标 " << target_id << " 没有活跃战斗" << std::endl;
        sendPacket(client_socket, PACKET_SPECTATE_JOIN, "0|||0|");
        return;
    }

    // 构建响应（使用统一的分隔符格式）
    // 格式："1|attackerId|defenderId|elapsedMs|mapData[[[HISTORY]]]action1[[[ACTION]]]action2..."
    std::ostringstream oss;
    oss << "1|" << attacker_id << "|" << defender_id << "|" << elapsed_ms << "|"
        << map_data;

    if (!history.empty()) {
        oss << "[[[HISTORY]]]";
        for (size_t i = 0; i < history.size(); ++i) {
            if (i > 0) {
                oss << "[[[ACTION]]]";
            }
            oss << history[i];
        }
    }

    sendPacket(client_socket, PACKET_SPECTATE_JOIN, oss.str());
}

void ArenaSession::EndSession(const std::string& attacker_id) {
    PvpSession session_copy;
    bool found = false;
    std::vector<std::string> spectator_ids_copy;
    std::string defender_id;
    SOCKET defender_socket = INVALID_SOCKET;
    std::vector<std::pair<std::string, SOCKET>> spectators_to_notify;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        auto it = sessions_.find(attacker_id);
        if (it == sessions_.end()) {
            std::cout << "[PVP] EndSession: 会话 " << attacker_id << " 不存在" << std::endl;
            return;
        }

        // 先标记为非活跃，防止并发问题
        it->second.isActive = false;
        session_copy = it->second;
        defender_id = session_copy.defenderId;
        
        // 收集需要通知的socket（在锁内收集）
        PlayerContext* defender = player_registry_->GetById(defender_id);
        if (defender != nullptr && defender->socket != INVALID_SOCKET) {
            defender_socket = defender->socket;
        }

        for (const auto& spectator_id : session_copy.spectatorIds) {
            PlayerContext* spectator = player_registry_->GetById(spectator_id);
            if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                spectators_to_notify.push_back({spectator_id, spectator->socket});
            }
        }

        sessions_.erase(it);
        found = true;

        std::cout << "[PVP] 会话结束: " << attacker_id 
                  << " (防守方: " << defender_id 
                  << ", 观战者: " << spectators_to_notify.size() << "人)" 
                  << std::endl;
    }

    if (!found) {
        return;
    }

    // 在锁外发送网络包，防止死锁
    // 通知防守方
    if (defender_socket != INVALID_SOCKET) {
        sendPacket(defender_socket, PACKET_PVP_END, "BATTLE_ENDED");
        std::cout << "[PVP] 已通知防守方: " << defender_id << std::endl;
    }

    // 通知所有观战者
    for (const auto& pair : spectators_to_notify) {
        sendPacket(pair.second, PACKET_PVP_END, "BATTLE_ENDED");
        std::cout << "[PVP] 已通知观战者: " << pair.first << std::endl;
    }

    BroadcastBattleStatusToAll();
}

void ArenaSession::CleanupPlayerSessions(const std::string& player_id) {
    std::vector<std::pair<std::string, SOCKET>> defenders_to_notify;
    std::vector<std::pair<std::string, SOCKET>> spectators_to_notify;
    std::vector<std::pair<std::string, SOCKET>> attackers_to_notify;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        // 清理玩家作为攻击者的会话
        auto it = sessions_.find(player_id);
        if (it != sessions_.end()) {
            std::cout << "[PVP] 清理攻击者会话: " << player_id << std::endl;

            PvpSession& session = it->second;
            session.isActive = false;

            // 收集防守方
            PlayerContext* defender =
                player_registry_->GetById(session.defenderId);
            if (defender != nullptr && defender->socket != INVALID_SOCKET) {
                defenders_to_notify.push_back({session.defenderId, defender->socket});
            }

            // 收集观战者
            for (const auto& spectator_id : session.spectatorIds) {
                PlayerContext* spectator = player_registry_->GetById(spectator_id);
                if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                    spectators_to_notify.push_back({spectator_id, spectator->socket});
                }
            }

            sessions_.erase(it);
        }

        // 清理玩家作为防守者或观战者的会话
        for (auto session_it = sessions_.begin(); session_it != sessions_.end();) {
            auto& session = session_it->second;

            // 从观战者列表中移除
            auto spectator_it = std::find(session.spectatorIds.begin(),
                                           session.spectatorIds.end(), player_id);
            if (spectator_it != session.spectatorIds.end()) {
                session.spectatorIds.erase(spectator_it);
                std::cout << "[PVP] 从会话中移除观战者: " << player_id << std::endl;
            }

            // 如果防守者断开连接，结束会话
            if (session.defenderId == player_id) {
                std::cout << "[PVP] 防守者断开连接，结束会话: " << session_it->first
                          << std::endl;

                session.isActive = false;

                // 收集攻击方
                PlayerContext* attacker =
                    player_registry_->GetById(session.attackerId);
                if (attacker != nullptr && attacker->socket != INVALID_SOCKET) {
                    attackers_to_notify.push_back({session.attackerId, attacker->socket});
                }

                // 收集观战者
                for (const auto& spectator_id : session.spectatorIds) {
                    PlayerContext* spectator =
                        player_registry_->GetById(spectator_id);
                    if (spectator != nullptr &&
                        spectator->socket != INVALID_SOCKET) {
                        spectators_to_notify.push_back({spectator_id, spectator->socket});
                    }
                }

                session_it = sessions_.erase(session_it);
            } else {
                ++session_it;
            }
        }
    }

    // 在锁外发送网络包，防止死锁
    for (const auto& pair : defenders_to_notify) {
        sendPacket(pair.second, PACKET_PVP_END, "OPPONENT_DISCONNECTED");
        std::cout << "[PVP] 通知防守方攻击者断开: " << pair.first << std::endl;
    }

    for (const auto& pair : attackers_to_notify) {
        sendPacket(pair.second, PACKET_PVP_END, "DEFENDER_DISCONNECTED");
        std::cout << "[PVP] 通知攻击方防守者断开: " << pair.first << std::endl;
    }

    for (const auto& pair : spectators_to_notify) {
        sendPacket(pair.second, PACKET_PVP_END, "BATTLE_ENDED");
        std::cout << "[PVP] 通知观战者战斗结束: " << pair.first << std::endl;
    }

    // 广播最新战斗状态
    BroadcastBattleStatusToAll();
}

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

        if (!first) {
            oss << ",";
        }
        first = false;

        // 攻击者状态
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