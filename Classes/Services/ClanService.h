/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanService.h
 * File Function: 部落服务层 - 处理网络通信和业务逻辑
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __CLAN_SERVICE_H__
#define __CLAN_SERVICE_H__

#include "ClanDataCache.h"
#include <functional>
#include <string>

// 操作结果回调
using OperationCallback = std::function<void(bool success, const std::string& message)>;

class ClanService
{
public:
    static ClanService& getInstance();

    ClanService(const ClanService&)            = delete;
    ClanService& operator=(const ClanService&) = delete;

    // 连接服务器
    void connect(const std::string& ip, int port, OperationCallback callback);
    bool isConnected() const;

    // 数据请求
    void requestOnlinePlayers();
    void requestClanMembers();
    void requestClanList();
    void requestBattleStatus();

    // 部落操作
    void createClan(const std::string& clanName, OperationCallback callback);
    void joinClan(const std::string& clanId, OperationCallback callback);
    void leaveClan(OperationCallback callback); // 🆕 退出部落

    // 初始化（注册网络回调）
    void initialize();
    void cleanup();

    // 同步本地账户的部落信息
    void syncLocalClanInfo();

private:
    ClanService() = default;

    void registerNetworkCallbacks();
    void parseUserListData(const std::string& data);
    void parseClanMembersData(const std::string& json);
    void parseBattleStatusData(const std::string& json);

    // 临时回调存储
    OperationCallback _connectCallback;
    OperationCallback _createClanCallback;
    OperationCallback _joinClanCallback;
    OperationCallback _leaveClanCallback; // 🆕 退出部落回调
    std::string       _pendingClanId;
    std::string       _pendingClanName;

    bool _initialized = false;
};

#endif // __CLAN_SERVICE_H__