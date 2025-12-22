/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanDataCache.h
 * File Function: 部落数据缓存 - 统一管理部落相关数据
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __CLAN_DATA_CACHE_H__
#define __CLAN_DATA_CACHE_H__

#include "Managers/SocketClient.h"
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

// 玩家战斗状态
struct PlayerBattleStatus
{
    bool        isInBattle   = false;
    std::string opponentId;
    std::string opponentName;
    bool        isAttacker   = false;
};

// 在线玩家信息
struct OnlinePlayerInfo
{
    std::string userId;
    std::string username;
    int         thLevel = 1;
    int         gold    = 0;
    int         elixir  = 0;
};

// 部落成员信息
struct ClanMemberInfo
{
    std::string id;
    std::string name;
    int         trophies = 0;
    bool        isOnline = false;
};

// 部落战成员信息
struct ClanWarMemberInfo
{
    std::string userId;
    std::string username;
    int         bestStars       = 0;
    float       bestDestruction = 0.0f;
    bool        canAttack       = false;
};

// 数据变更通知类型
enum class ClanDataChangeType
{
    ONLINE_PLAYERS,
    CLAN_MEMBERS,
    CLAN_WAR_MEMBERS,
    BATTLE_STATUS,
    CLAN_LIST,
    CLAN_INFO
};

class ClanDataCache
{
public:
    static ClanDataCache& getInstance();

    // 禁止拷贝
    ClanDataCache(const ClanDataCache&)            = delete;
    ClanDataCache& operator=(const ClanDataCache&) = delete;

    // 数据访问
    const std::vector<OnlinePlayerInfo>&   getOnlinePlayers() const { return _onlinePlayers; }
    const std::vector<ClanMemberInfo>&     getClanMembers() const { return _clanMembers; }
    const std::vector<ClanWarMemberInfo>&  getClanWarMembers() const { return _clanWarMembers; }
    const std::vector<ClanInfoClient>&     getClanList() const { return _clanList; }
    const PlayerBattleStatus&              getBattleStatus(const std::string& playerId) const;
    bool                                   isPlayerInBattle(const std::string& playerId) const;

    // 当前部落信息
    const std::string& getCurrentClanId() const { return _currentClanId; }
    const std::string& getCurrentClanName() const { return _currentClanName; }
    bool               isInClan() const { return _isInClan; }
    const std::string& getCurrentWarId() const { return _currentWarId; }

    // 数据更新（由 Service 层调用）
    void setOnlinePlayers(const std::vector<OnlinePlayerInfo>& players);
    void setClanMembers(const std::vector<ClanMemberInfo>& members);
    void setClanWarMembers(const std::vector<ClanWarMemberInfo>& members);
    void setClanList(const std::vector<ClanInfoClient>& clans);
    void setBattleStatusMap(const std::map<std::string, PlayerBattleStatus>& statusMap);
    void setCurrentClan(const std::string& clanId, const std::string& clanName);
    void clearCurrentClan();
    void setCurrentWarId(const std::string& warId) { _currentWarId = warId; }

    // 辅助方法
    std::string findClanNameById(const std::string& clanId) const;

    // 观察者模式
    using DataChangeCallback = std::function<void(ClanDataChangeType)>;
    void addObserver(void* owner, DataChangeCallback callback);
    void removeObserver(void* owner);

private:
    ClanDataCache() = default;

    void notifyObservers(ClanDataChangeType type);

    // 缓存数据
    std::vector<OnlinePlayerInfo>          _onlinePlayers;
    std::vector<ClanMemberInfo>            _clanMembers;
    std::vector<ClanWarMemberInfo>         _clanWarMembers;
    std::vector<ClanInfoClient>            _clanList;
    std::map<std::string, PlayerBattleStatus> _battleStatusMap;
    std::set<std::string>                  _playersInBattle;

    // 当前部落状态
    std::string _currentClanId;
    std::string _currentClanName;
    bool        _isInClan = false;
    std::string _currentWarId;

    // 观察者
    std::map<void*, DataChangeCallback> _observers;

    // 默认空状态
    static const PlayerBattleStatus _emptyStatus;
};

#endif // __CLAN_DATA_CACHE_H__