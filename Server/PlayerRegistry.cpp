/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PlayerRegistry.cpp
 * File Function: 玩家注册管理实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "PlayerRegistry.h"

void PlayerRegistry::Register(SOCKET s, const PlayerContext& ctx) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    players_[s] = ctx;
}

void PlayerRegistry::Unregister(SOCKET s) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    players_.erase(s);
}

PlayerContext* PlayerRegistry::GetBySocket(SOCKET s) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = players_.find(s);
    return it != players_.end() ? &it->second : nullptr;
}

PlayerContext* PlayerRegistry::GetById(const std::string& player_id) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    for (auto& pair : players_) {
        if (pair.second.playerId == player_id) {
            return &pair.second;
        }
    }
    return nullptr;
}

std::map<SOCKET, PlayerContext> PlayerRegistry::GetAllSnapshot() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return players_;  // 返回副本
}