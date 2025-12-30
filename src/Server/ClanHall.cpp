/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanHall.cpp
 * File Function: 部落系统实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ClanHall.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// ============================================================================
// 构造函数
// ============================================================================

ClanHall::ClanHall(PlayerRegistry* registry) : player_registry_(registry) {}

// ============================================================================
// 私有辅助方法
// ============================================================================

std::string ClanHall::GenerateClanId() {
    static int counter = 0;
    return "CLAN_" + std::to_string(++counter);
}

// ============================================================================
// 部落创建与管理
// ============================================================================

bool ClanHall::CreateClan(const std::string& player_id,
                          const std::string& clan_name) {
    // 验证玩家存在性
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        std::cout << "[Clan] 创建失败: 玩家 " << player_id << " 未找到"
                  << std::endl;
        return false;
    }

    // 验证玩家未加入其他部落
    if (!player->clanId.empty()) {
        std::cout << "[Clan] 创建失败: " << player_id << " 已在部落中"
                  << std::endl;
        return false;
    }

    std::string clan_id = GenerateClanId();

    // 创建部落记录
    {
        std::lock_guard<std::mutex> lock(clan_mutex_);

        ClanInfo clan;
        clan.clanId = clan_id;
        clan.clanName = clan_name;
        clan.leaderId = player_id;
        clan.memberIds.push_back(player_id);
        clan.clanTrophies = player->trophies;
        clan.requiredTrophies = 0;
        clan.isOpen = true;

        clans_[clan_id] = clan;
    }

    // 更新玩家的部落归属
    player->clanId = clan_id;

    std::cout << "[Clan] 创建成功: " << clan_name << " (ID: " << clan_id
              << ") 创建者: " << player_id << std::endl;
    return true;
}

bool ClanHall::JoinClan(const std::string& player_id,
                        const std::string& clan_id) {
    // 验证玩家存在性
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        std::cout << "[Clan] 加入失败: 玩家 " << player_id << " 未找到"
                  << std::endl;
        return false;
    }

    // 验证玩家未加入其他部落
    if (!player->clanId.empty()) {
        std::cout << "[Clan] 加入失败: " << player_id << " 已在部落 "
                  << player->clanId << " 中" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(clan_mutex_);

    // 验证目标部落存在
    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        std::cout << "[Clan] 加入失败: 部落 " << clan_id << " 未找到"
                  << std::endl;
        return false;
    }

    // 验证部落开放状态
    if (!it->second.isOpen) {
        std::cout << "[Clan] 加入失败: 部落 " << clan_id << " 未开放"
                  << std::endl;
        return false;
    }

    // 验证奖杯数要求
    if (player->trophies < it->second.requiredTrophies) {
        std::cout << "[Clan] 加入失败: " << player_id << " 奖杯数 "
                  << player->trophies << " 不满足要求 "
                  << it->second.requiredTrophies << std::endl;
        return false;
    }

    // 执行加入操作
    it->second.memberIds.push_back(player_id);
    it->second.clanTrophies += player->trophies;
    player->clanId = clan_id;

    std::cout << "[Clan] " << player_id << " 加入 " << it->second.clanName
              << " (ID: " << clan_id << ")" << std::endl;
    return true;
}

bool ClanHall::LeaveClan(const std::string& player_id) {
    // 验证玩家存在性
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        return false;
    }

    // 验证玩家已加入部落
    std::string clan_id = player->clanId;
    if (clan_id.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(clan_mutex_);

    // 查找部落
    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return false;
    }

    // 从成员列表中移除
    auto& members = it->second.memberIds;
    members.erase(std::remove(members.begin(), members.end(), player_id),
                  members.end());
    it->second.clanTrophies -= player->trophies;
    player->clanId = "";

    // 如果部落为空，删除部落
    if (members.empty()) {
        clans_.erase(it);
        std::cout << "[Clan] 删除空部落: " << clan_id << std::endl;
    }

    std::cout << "[Clan] " << player_id << " 离开部落 " << clan_id << std::endl;
    return true;
}

// ============================================================================
// 部落查询
// ============================================================================

std::string ClanHall::GetClanListJson() {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    std::ostringstream oss;
    oss << "[";
    bool first = true;

    for (const auto& pair : clans_) {
        const auto& clan = pair.second;
        if (!first) {
            oss << ",";
        }
        first = false;

        oss << "{" << "\"id\":\"" << clan.clanId << "\",\""
            << "name\":\"" << clan.clanName << "\",\""
            << "members\":" << clan.memberIds.size() << ","
            << "\"trophies\":" << clan.clanTrophies << ","
            << "\"required\":" << clan.requiredTrophies << ","
            << "\"open\":" << (clan.isOpen ? "true" : "false") << "}";
    }

    oss << "]";
    return oss.str();
}

std::string ClanHall::GetClanMembersJson(const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return "{\"error\":\"CLAN_NOT_FOUND\"}";
    }

    std::ostringstream oss;
    oss << "{\"members\":[";

    bool first = true;
    for (const auto& member_id : it->second.memberIds) {
        if (!first) {
            oss << ",";
        }
        first = false;

        // 获取成员的在线状态和信息
        PlayerContext* player = player_registry_->GetById(member_id);
        bool online = (player != nullptr);
        int trophies = online ? player->trophies : 0;
        std::string name = online ? player->playerName : member_id;

        oss << "{" << "\"id\":\"" << member_id << "\",\""
            << "name\":\"" << name << "\",\""
            << "trophies\":" << trophies << ","
            << "\"online\":" << (online ? "true" : "false") << "}";
    }

    oss << "]}";
    return oss.str();
}

bool ClanHall::IsPlayerInClan(const std::string& player_id,
                              const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return false;
    }

    auto& members = it->second.memberIds;
    return std::find(members.begin(), members.end(), player_id) != members.end();
}

std::vector<std::string> ClanHall::GetClanMemberIds(const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return {};
    }

    return it->second.memberIds;  // 返回副本
}