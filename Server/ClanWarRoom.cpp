/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanWarRoom.cpp
 * File Function: 部落战争系统实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ClanWarRoom.h"

#include "Protocol.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// 前向声明网络发送函数
extern bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

ClanWarRoom::ClanWarRoom(PlayerRegistry* registry, ClanHall* hall)
    : player_registry_(registry), clan_hall_(hall) {}

std::string ClanWarRoom::GenerateWarId() {
    static int counter = 0;
    return "WAR_" + std::to_string(++counter);
}

void ClanWarRoom::AddToQueue(const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(war_mutex_);

    // 检查部落是否已在队列中
    if (std::find(war_queue_.begin(), war_queue_.end(), clan_id) !=
        war_queue_.end()) {
        return;
    }

    // 检查部落是否已在活跃战争中
    {
        std::lock_guard<std::mutex> session_lock(session_mutex_);
        for (const auto& war_pair : active_wars_) {
            if (war_pair.second.clan1Id == clan_id ||
                war_pair.second.clan2Id == clan_id) {
                std::cout << "[ClanWar] 部落 " << clan_id << " 已在战争中"
                          << std::endl;
                return;
            }
        }
    }

    war_queue_.push_back(clan_id);
    std::cout << "[ClanWar] 部落 " << clan_id << " 加入匹配队列" << std::endl;

    ProcessQueue();
}

void ClanWarRoom::ProcessQueue() {
    // 假设已持有 war_mutex_
    if (war_queue_.size() < 2) {
        return;
    }

    std::string clan1_id = war_queue_[0];
    std::string clan2_id = war_queue_[1];

    war_queue_.erase(war_queue_.begin());
    war_queue_.erase(war_queue_.begin());

    StartWar(clan1_id, clan2_id);
}

void ClanWarRoom::StartWar(const std::string& clan1_id,
                           const std::string& clan2_id) {
    std::string war_id = GenerateWarId();

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        ClanWarSession session;
        session.warId = war_id;
        session.clan1Id = clan1_id;
        session.clan2Id = clan2_id;
        session.startTime = std::chrono::steady_clock::now();
        session.isActive = true;
        session.clan1TotalStars = 0;
        session.clan2TotalStars = 0;

        // 初始化第一个部落的成员
        auto clan1_members = clan_hall_->GetClanMemberIds(clan1_id);
        for (const auto& member_id : clan1_members) {
            ClanWarMember member;
            member.memberId = member_id;
            member.bestStars = 0;
            member.bestDestructionRate = 0.0f;

            PlayerContext* player = player_registry_->GetById(member_id);
            if (player != nullptr) {
                member.memberName = player->playerName;
                member.mapData = player->mapData;
            }

            session.clan1Members.push_back(member);
        }

        // 初始化第二个部落的成员
        auto clan2_members = clan_hall_->GetClanMemberIds(clan2_id);
        for (const auto& member_id : clan2_members) {
            ClanWarMember member;
            member.memberId = member_id;
            member.bestStars = 0;
            member.bestDestructionRate = 0.0f;

            PlayerContext* player = player_registry_->GetById(member_id);
            if (player != nullptr) {
                member.memberName = player->playerName;
                member.mapData = player->mapData;
            }

            session.clan2Members.push_back(member);
        }

        active_wars_[war_id] = session;

        std::cout << "[ClanWar] 战争开始: " << war_id << " (" << clan1_id
                  << " vs " << clan2_id << ")" << std::endl;
    }

    // 通知所有成员战争已开始
    std::string msg = war_id + "|" + clan1_id + "|" + clan2_id;

    auto notify_members = [&](const std::vector<std::string>& member_ids) {
        for (const auto& member_id : member_ids) {
            PlayerContext* player = player_registry_->GetById(member_id);
            if (player != nullptr && player->socket != INVALID_SOCKET) {
                sendPacket(player->socket, PACKET_CLAN_WAR_MATCH, msg);
            }
        }
    };

    notify_members(clan_hall_->GetClanMemberIds(clan1_id));
    notify_members(clan_hall_->GetClanMemberIds(clan2_id));
}

void ClanWarRoom::HandleAttackStart(SOCKET client_socket,
                                    const std::string& war_id,
                                    const std::string& target_id) {
    PlayerContext* attacker = player_registry_->GetBySocket(client_socket);
    if (attacker == nullptr) {
        sendPacket(client_socket, PACKET_CLAN_WAR_ATTACK_START,
                   "FAIL|NOT_LOGGED_IN|");
        return;
    }

    std::string attacker_id = attacker->playerId;
    std::string target_map_data;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        auto it = active_wars_.find(war_id);
        if (it == active_wars_.end()) {
            sendPacket(client_socket, PACKET_CLAN_WAR_ATTACK_START,
                       "FAIL|WAR_NOT_FOUND|");
            return;
        }

        ClanWarSession& session = it->second;

        // 检查战争是否还在进行中
        if (!session.isActive) {
            sendPacket(client_socket, PACKET_CLAN_WAR_ATTACK_START,
                       "FAIL|WAR_ENDED|");
            return;
        }

        // 检查攻击者是否已在战斗中
        if (session.activeBattles.find(attacker_id) !=
            session.activeBattles.end()) {
            sendPacket(client_socket, PACKET_CLAN_WAR_ATTACK_START,
                       "FAIL|ALREADY_IN_BATTLE|");
            return;
        }

        // 查找目标成员
        ClanWarMember* target_member = nullptr;
        for (auto& member : session.clan1Members) {
            if (member.memberId == target_id) {
                target_member = &member;
                break;
            }
        }
        if (target_member == nullptr) {
            for (auto& member : session.clan2Members) {
                if (member.memberId == target_id) {
                    target_member = &member;
                    break;
                }
            }
        }

        if (target_member == nullptr || target_member->mapData.empty()) {
            sendPacket(client_socket, PACKET_CLAN_WAR_ATTACK_START,
                       "FAIL|NO_MAP_DATA|");
            return;
        }

        target_map_data = target_member->mapData;

        // 创建战斗会话
        PvpSession pvp_session;
        pvp_session.attackerId = attacker_id;
        pvp_session.defenderId = target_id;
        pvp_session.mapData = target_map_data;
        pvp_session.isActive = true;
        pvp_session.startTime = std::chrono::steady_clock::now();

        session.activeBattles[attacker_id] = pvp_session;
    }

    std::cout << "[ClanWar] 攻击开始: " << attacker_id << " -> " << target_id
              << " (战争: " << war_id << ")" << std::endl;

    std::string response = "ATTACK|" + target_id + "|" + target_map_data;
    sendPacket(client_socket, PACKET_CLAN_WAR_ATTACK_START, response);
}

void ClanWarRoom::HandleAttackEnd(const std::string& war_id,
                                  const AttackRecord& record) {
    bool need_broadcast = false;
    std::string defender_id;
    SOCKET defender_socket = INVALID_SOCKET;
    std::vector<std::pair<std::string, SOCKET>> spectators_to_notify;
    size_t total_action_count = 0;  // 🔧 新增：总操作数量

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        auto it = active_wars_.find(war_id);
        if (it == active_wars_.end()) {
            std::cout << "[ClanWar] 错误: 战争 " << war_id << " 未找到"
                      << std::endl;
            return;
        }

        ClanWarSession& session = it->second;

        // 查找并验证战斗会话
        auto battle_it = session.activeBattles.find(record.attackerId);
        if (battle_it == session.activeBattles.end()) {
            std::cout << "[ClanWar] 错误: 玩家 " << record.attackerId
                      << " 没有活跃的战斗" << std::endl;
            return;
        }

        defender_id = battle_it->second.defenderId;
        total_action_count = battle_it->second.actionHistory.size();  // 🔧 获取总操作数
        
        // 🔧 修复：收集需要通知的观战者（在锁内收集socket）
        for (const auto& spectator_id : battle_it->second.spectatorIds) {
            PlayerContext* spectator = player_registry_->GetById(spectator_id);
            if (spectator != nullptr && spectator->socket != INVALID_SOCKET) {
                spectators_to_notify.push_back({spectator_id, spectator->socket});
            }
        }

        // 判断攻击者所属部落
        bool is_attacker_in_clan1 =
            clan_hall_->IsPlayerInClan(record.attackerId, session.clan1Id);

        // 目标在敌方部落
        auto& target_members =
            is_attacker_in_clan1 ? session.clan2Members : session.clan1Members;

        ClanWarMember* target_member = nullptr;
        for (auto& member : target_members) {
            if (member.memberId == defender_id) {
                target_member = &member;
                break;
            }
        }

        if (target_member != nullptr) {
            // 记录攻击
            target_member->attacksReceived.push_back(record);

            // 更新最佳成绩
            if (record.starsEarned > target_member->bestStars ||
                (record.starsEarned == target_member->bestStars &&
                 record.destructionRate > target_member->bestDestructionRate)) {
                target_member->bestStars = record.starsEarned;
                target_member->bestDestructionRate = record.destructionRate;
            }

            // 更新部落总星数
            if (is_attacker_in_clan1) {
                session.clan1TotalStars += record.starsEarned;
            } else {
                session.clan2TotalStars += record.starsEarned;
            }

            std::cout << "[ClanWar] 攻击结束: " << record.attackerId << " -> "
                      << defender_id << " (获得 " << record.starsEarned
                      << " 星, 总操作数: " << total_action_count << ")"
                      << std::endl;
        }

        // 获取防守方socket（在锁内）
        PlayerContext* defender = player_registry_->GetById(defender_id);
        if (defender != nullptr && defender->socket != INVALID_SOCKET) {
            defender_socket = defender->socket;
        }

        // 清理战斗会话
        session.activeBattles.erase(record.attackerId);
        need_broadcast = true;
    }

    // 🔧 修复：构建包含总操作数的结束消息
    std::ostringstream end_msg;
    end_msg << "BATTLE_ENDED|" << total_action_count;
    std::string end_message = end_msg.str();

    // 🔧 修复：在锁外发送网络包，防止死锁
    // 通知防守方战斗结束
    if (defender_socket != INVALID_SOCKET) {
        sendPacket(defender_socket, PACKET_CLAN_WAR_ATTACK_END, end_message);
    }

    // 通知观战者
    for (const auto& pair : spectators_to_notify) {
        sendPacket(pair.second, PACKET_CLAN_WAR_ATTACK_END, end_message);
        std::cout << "[ClanWar] 已通知观战者: " << pair.first 
                  << " (总操作数: " << total_action_count << ")" << std::endl;
    }

    if (need_broadcast) {
        BroadcastWarUpdate(war_id);
    }
}

void ClanWarRoom::HandleSpectate(SOCKET client_socket,
                                 const std::string& war_id,
                                 const std::string& target_id) {
    PlayerContext* spectator = player_registry_->GetBySocket(client_socket);
    if (spectator == nullptr) {
        sendPacket(client_socket, PACKET_CLAN_WAR_SPECTATE, "0|||");
        return;
    }

    std::string spectator_id = spectator->playerId;
    std::string attacker_id, defender_id, map_data;
    std::vector<std::string> history;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        auto it = active_wars_.find(war_id);
        if (it == active_wars_.end()) {
            sendPacket(client_socket, PACKET_CLAN_WAR_SPECTATE, "0|||");
            return;
        }

        ClanWarSession& session = it->second;

        for (auto& pair : session.activeBattles) {
            if (!pair.second.isActive) {
                continue;
            }

            if (pair.second.attackerId == target_id ||
                pair.second.defenderId == target_id) {
                attacker_id = pair.second.attackerId;
                defender_id = pair.second.defenderId;
                map_data = pair.second.mapData;
                history = pair.second.actionHistory;
                
                // 🔧 修复：防止重复添加观战者
                auto& spectators = pair.second.spectatorIds;
                if (std::find(spectators.begin(), spectators.end(), spectator_id) == spectators.end()) {
                    spectators.push_back(spectator_id);
                }
                found = true;
                break;
            }
        }
    }

    if (!found || map_data.empty()) {
        sendPacket(client_socket, PACKET_CLAN_WAR_SPECTATE, "0|||");
        return;
    }

    std::cout << "[ClanWar] 观战者 " << spectator_id << " 正在观看 "
              << attacker_id << " vs " << defender_id 
              << " (历史操作: " << history.size() << ")" << std::endl;

    // 🔧 修复：构建包含历史记录的响应
    std::ostringstream oss;
    oss << "1|" << attacker_id << "|" << defender_id << "|" << map_data;
    
    if (!history.empty()) {
        oss << "[[[HISTORY]]]";
        for (size_t i = 0; i < history.size(); ++i) {
            if (i > 0) {
                oss << "[[[ACTION]]]";
            }
            oss << history[i];
        }
    }

    sendPacket(client_socket, PACKET_CLAN_WAR_SPECTATE, oss.str());
}

void ClanWarRoom::EndWar(const std::string& war_id) {
    std::string result_json;
    std::string clan1_id, clan2_id;
    std::vector<std::string> all_member_ids;
    std::vector<std::pair<SOCKET, std::string>> packets_to_send;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        auto it = active_wars_.find(war_id);
        if (it == active_wars_.end()) {
            std::cout << "[ClanWar] 错误: 战争 " << war_id << " 未找到"
                      << std::endl;
            return;
        }

        ClanWarSession& session = it->second;
        
        // 🔧 修复：先标记为非活跃状态，防止并发问题
        session.isActive = false;

        // 检查是否还有活跃战斗
        if (!session.activeBattles.empty()) {
            std::cout << "[ClanWar] 警告: 战争 " << war_id
                      << " 仍有 " << session.activeBattles.size()
                      << " 场活跃战斗，正在强制结束" << std::endl;

            // 🔧 修复：收集所有需要通知的socket（在锁内收集）
            for (auto& battle_pair : session.activeBattles) {
                battle_pair.second.isActive = false;
                
                PlayerContext* attacker =
                    player_registry_->GetById(battle_pair.second.attackerId);
                if (attacker != nullptr && attacker->socket != INVALID_SOCKET) {
                    packets_to_send.push_back({attacker->socket, "WAR_ENDED"});
                }
                
                // 通知观战者
                for (const auto& spectator_id : battle_pair.second.spectatorIds) {
                    PlayerContext* spectator =
                        player_registry_->GetById(spectator_id);
                    if (spectator != nullptr &&
                        spectator->socket != INVALID_SOCKET) {
                        packets_to_send.push_back({spectator->socket, "WAR_ENDED"});
                    }
                }
            }
            session.activeBattles.clear();
        }

        session.endTime = std::chrono::steady_clock::now();

        clan1_id = session.clan1Id;
        clan2_id = session.clan2Id;

        // 确定胜者
        std::string winner_id;
        if (session.clan1TotalStars > session.clan2TotalStars) {
            winner_id = session.clan1Id;
        } else if (session.clan2TotalStars > session.clan1TotalStars) {
            winner_id = session.clan2Id;
        }
        // 如果星数相同则为平局，winner_id 保持为空

        // 构建结果JSON
        std::ostringstream oss;
        oss << "{";
        oss << "\"warId\":\"" << war_id << "\",";
        oss << "\"clan1Id\":\"" << clan1_id << "\",";
        oss << "\"clan2Id\":\"" << clan2_id << "\",";
        oss << "\"clan1Stars\":" << session.clan1TotalStars << ",";
        oss << "\"clan2Stars\":" << session.clan2TotalStars << ",";
        oss << "\"winnerId\":\"" << winner_id << "\"";
        oss << "}";
        result_json = oss.str();

        // 收集所有成员ID用于通知
        for (const auto& member : session.clan1Members) {
            all_member_ids.push_back(member.memberId);
        }
        for (const auto& member : session.clan2Members) {
            all_member_ids.push_back(member.memberId);
        }

        // 从活跃战争中移除
        active_wars_.erase(it);

        std::cout << "[ClanWar] 战争结束: " << war_id << " (胜者: "
                  << (winner_id.empty() ? "平局" : winner_id) << ")"
                  << std::endl;
    }

    // 🔧 修复：在锁外发送网络包，防止死锁
    // 首先发送战斗结束通知给还在战斗中的玩家
    for (const auto& packet : packets_to_send) {
        sendPacket(packet.first, PACKET_CLAN_WAR_ATTACK_END, packet.second);
    }

    // 通知所有参与者战争结束
    for (const auto& member_id : all_member_ids) {
        PlayerContext* player = player_registry_->GetById(member_id);
        if (player != nullptr && player->socket != INVALID_SOCKET) {
            sendPacket(player->socket, PACKET_CLAN_WAR_END, result_json);
        }
    }
}

void ClanWarRoom::CleanupPlayerSessions(const std::string& player_id) {
    std::vector<std::pair<SOCKET, std::string>> packets_to_send;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        for (auto& war_pair : active_wars_) {
            ClanWarSession& session = war_pair.second;

            // 清理玩家作为攻击者的战斗
            auto battle_it = session.activeBattles.find(player_id);
            if (battle_it != session.activeBattles.end()) {
                std::cout << "[ClanWar] 清理玩家 " << player_id << " 的攻击会话"
                          << std::endl;

                battle_it->second.isActive = false;

                // 🔧 修复：收集需要通知的socket
                for (const auto& spectator_id : battle_it->second.spectatorIds) {
                    PlayerContext* spectator =
                        player_registry_->GetById(spectator_id);
                    if (spectator != nullptr &&
                        spectator->socket != INVALID_SOCKET) {
                        packets_to_send.push_back({spectator->socket, "0|||"});
                    }
                }

                session.activeBattles.erase(battle_it);
            }

            // 从所有战斗的观战者列表中移除玩家
            for (auto& battle_pair : session.activeBattles) {
                auto& spectators = battle_pair.second.spectatorIds;
                spectators.erase(
                    std::remove(spectators.begin(), spectators.end(), player_id),
                    spectators.end());
            }
        }
    }

    // 🔧 修复：在锁外发送网络包
    for (const auto& packet : packets_to_send) {
        sendPacket(packet.first, PACKET_CLAN_WAR_SPECTATE, packet.second);
    }
}

std::string ClanWarRoom::GetActiveWarIdForPlayer(const std::string& player_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);

    for (const auto& war_pair : active_wars_) {
        const ClanWarSession& session = war_pair.second;
        if (!session.isActive) {
            continue;
        }

        // 检查玩家是否在任一部落中
        for (const auto& member : session.clan1Members) {
            if (member.memberId == player_id) {
                return war_pair.first;
            }
        }
        for (const auto& member : session.clan2Members) {
            if (member.memberId == player_id) {
                return war_pair.first;
            }
        }
    }

    return "";
}

std::string ClanWarRoom::GetMemberListJson(const std::string& war_id,
                                           const std::string& requester_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);

    auto it = active_wars_.find(war_id);
    if (it == active_wars_.end()) {
        return "{\"error\":\"War not found\"}";
    }

    const ClanWarSession& session = it->second;

    bool is_in_clan1 =
        clan_hall_->IsPlayerInClan(requester_id, session.clan1Id);

    std::ostringstream oss;
    oss << "{";
    oss << "\"warId\":\"" << war_id << "\",";
    oss << "\"clan1TotalStars\":" << session.clan1TotalStars << ",";
    oss << "\"clan2TotalStars\":" << session.clan2TotalStars << ",";
    oss << "\"isActive\":" << (session.isActive ? "true" : "false") << ",";
    oss << "\"enemyMembers\":[";

    const auto& enemy_members =
        is_in_clan1 ? session.clan2Members : session.clan1Members;
    bool first = true;
    for (const auto& member : enemy_members) {
        if (!first) {
            oss << ",";
        }
        first = false;

        oss << "{";
        oss << "\"id\":\"" << member.memberId << "\",";
        oss << "\"name\":\"" << member.memberName << "\",";
        oss << "\"bestStars\":" << member.bestStars << ",";
        oss << "\"bestDestruction\":" << member.bestDestructionRate << ",";
        oss << "\"canAttack\":" << (!member.mapData.empty() ? "true" : "false");
        oss << "}";
    }

    oss << "]}";
    return oss.str();
}

void ClanWarRoom::BroadcastWarUpdate(const std::string& war_id) {
    std::string state_json;
    std::string clan1_id, clan2_id;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        auto it = active_wars_.find(war_id);
        if (it == active_wars_.end()) {
            return;
        }

        const ClanWarSession& session = it->second;
        clan1_id = session.clan1Id;
        clan2_id = session.clan2Id;

        std::ostringstream oss;
        oss << "{";
        oss << "\"warId\":\"" << war_id << "\",";
        oss << "\"clan1Stars\":" << session.clan1TotalStars << ",";
        oss << "\"clan2Stars\":" << session.clan2TotalStars;
        oss << "}";

        state_json = oss.str();
    }

    auto send_to_members = [&](const std::vector<std::string>& member_ids) {
        for (const auto& member_id : member_ids) {
            PlayerContext* player = player_registry_->GetById(member_id);
            if (player != nullptr && player->socket != INVALID_SOCKET) {
                sendPacket(player->socket, PACKET_CLAN_WAR_STATE_UPDATE,
                           state_json);
            }
        }
    };

    send_to_members(clan_hall_->GetClanMemberIds(clan1_id));
    send_to_members(clan_hall_->GetClanMemberIds(clan2_id));
}

void ClanWarRoom::BroadcastWarEnd(const std::string& war_id,
                                  const std::string& result_json) {
    std::string clan1_id, clan2_id;

    {
        std::lock_guard<std::mutex> lock(session_mutex_);

        auto it = active_wars_.find(war_id);
        if (it == active_wars_.end()) {
            return;
        }

        clan1_id = it->second.clan1Id;
        clan2_id = it->second.clan2Id;
    }

    auto send_to_members = [&](const std::vector<std::string>& member_ids) {
        for (const auto& member_id : member_ids) {
            PlayerContext* player = player_registry_->GetById(member_id);
            if (player != nullptr && player->socket != INVALID_SOCKET) {
                sendPacket(player->socket, PACKET_CLAN_WAR_END, result_json);
            }
        }
    };

    send_to_members(clan_hall_->GetClanMemberIds(clan1_id));
    send_to_members(clan_hall_->GetClanMemberIds(clan2_id));
}