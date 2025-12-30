/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.h
 * File Function: 服务器主逻辑声明
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef SERVER_H_
#define SERVER_H_

#include "ArenaSession.h"
#include "ClanHall.h"
#include "ClanInfo.h"
#include "ClanWarRoom.h"
#include "CommandDispatcher.h"
#include "MatchMaker.h"
#include "PlayerRegistry.h"
#include "Protocol.h"
#include "WarModels.h"

#include <WinSock2.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

/**
 * @class Server
 * @brief 游戏服务器主类，管理网络连接和各个子系统
 */
class Server {
 public:
    Server();
    ~Server();

    /**
     * @brief 启动服务器
     */
    void run();

    // 友元函数，用于客户端连接处理
    friend void clientHandler(SOCKET clientSocket, Server& server);

 private:
    // ==================== 网络基础 ====================
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    int port;

    // ==================== 模块化组件 ====================
    std::unique_ptr<PlayerRegistry> playerRegistry;  // 玩家注册管理
    std::unique_ptr<ClanHall> clanHall;              // 部落系统
    std::unique_ptr<ClanWarRoom> clanWarRoom;        // 部落战争系统
    std::unique_ptr<Matchmaker> matchmaker;          // 匹配系统
    std::unique_ptr<ArenaSession> arenaSession;      // PVP竞技场
    std::unique_ptr<Router> router;                  // 命令路由器

    // ==================== 共享数据 ====================
    std::map<std::string, std::string> savedMaps;  // 玩家ID -> 地图数据
    std::map<std::string, PlayerContext> playerDatabase;  // 玩家持久化数据
    std::mutex dataMutex;  // 保护共享数据的互斥锁

    // ==================== 网络函数 ====================
    void createAndBindSocket();
    void handleConnections();
    void closeClientSocket(SOCKET clientSocket);

    // ==================== 路由注册 ====================
    void registerRoutes();

    // ==================== 辅助函数 ====================
    std::string serializeAttackResult(const AttackResult& result);
    AttackResult deserializeAttackResult(const std::string& data);
    std::string getUserListJson(const std::string& requesterId);
};

#endif  // SERVER_H_