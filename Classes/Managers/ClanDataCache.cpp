/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanDataCache.cpp
 * File Function: 部落数据缓存实现
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#include "ClanDataCache.h"

const PlayerBattleStatus ClanDataCache::_emptyStatus;

ClanDataCache& ClanDataCache::getInstance()
{
    static ClanDataCache instance;
    return instance;
}

const PlayerBattleStatus& ClanDataCache::getBattleStatus(const std::string& playerId) const
{
    auto it = _battleStatusMap.find(playerId);
    return (it != _battleStatusMap.end()) ? it->second : _emptyStatus;
}

bool ClanDataCache::isPlayerInBattle(const std::string& playerId) const
{
    return _playersInBattle.find(playerId) != _playersInBattle.end();
}

void ClanDataCache::setOnlinePlayers(const std::vector<OnlinePlayerInfo>& players)
{
    _onlinePlayers = players;
    notifyObservers(ClanDataChangeType::ONLINE_PLAYERS);
}

void ClanDataCache::setClanMembers(const std::vector<ClanMemberInfo>& members)
{
    _clanMembers = members;
    notifyObservers(ClanDataChangeType::CLAN_MEMBERS);
}

void ClanDataCache::setClanWarMembers(const std::vector<ClanWarMemberInfo>& members)
{
    _clanWarMembers = members;
    notifyObservers(ClanDataChangeType::CLAN_WAR_MEMBERS);
}

void ClanDataCache::setClanList(const std::vector<ClanInfoClient>& clans)
{
    _clanList = clans;
    
    // 更新当前部落名称
    if (!_currentClanId.empty())
    {
        for (const auto& clan : clans)
        {
            if (clan.clanId == _currentClanId)
            {
                _currentClanName = clan.clanName;
                break;
            }
        }
    }
    
    notifyObservers(ClanDataChangeType::CLAN_LIST);
}

void ClanDataCache::setBattleStatusMap(const std::map<std::string, PlayerBattleStatus>& statusMap)
{
    _battleStatusMap = statusMap;
    _playersInBattle.clear();
    
    for (const auto& pair : statusMap)
    {
        if (pair.second.isInBattle)
        {
            _playersInBattle.insert(pair.first);
        }
    }
    
    notifyObservers(ClanDataChangeType::BATTLE_STATUS);
}

void ClanDataCache::setCurrentClan(const std::string& clanId, const std::string& clanName)
{
    _currentClanId   = clanId;
    _currentClanName = clanName;
    _isInClan        = !clanId.empty();
    notifyObservers(ClanDataChangeType::CLAN_INFO);
}

void ClanDataCache::clearCurrentClan()
{
    _currentClanId.clear();
    _currentClanName.clear();
    _isInClan = false;
    notifyObservers(ClanDataChangeType::CLAN_INFO);
}

std::string ClanDataCache::findClanNameById(const std::string& clanId) const
{
    for (const auto& clan : _clanList)
    {
        if (clan.clanId == clanId)
        {
            return clan.clanName;
        }
    }
    return "";
}

void ClanDataCache::addObserver(void* owner, DataChangeCallback callback)
{
    _observers[owner] = callback;
}

void ClanDataCache::removeObserver(void* owner)
{
    _observers.erase(owner);
}

void ClanDataCache::notifyObservers(ClanDataChangeType type)
{
    for (const auto& pair : _observers)
    {
        if (pair.second)
        {
            pair.second(type);
        }
    }
}