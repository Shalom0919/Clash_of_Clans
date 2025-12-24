/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     CommandDispatcher.h
 * File Function: 命令路由分发器
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <WinSock2.h>

#include <cstdint>
#include <functional>
#include <map>
#include <string>

// 数据包处理函数类型
using PacketHandler = std::function<void(SOCKET, const std::string&)>;

/**
 * @class Router
 * @brief 管理数据包类型到处理函数的映射和路由分发
 */
class Router {
 public:
    /**
     * @brief 注册数据包处理函数
     * @param packet_type 数据包类型
     * @param handler 处理函数
     */
    void Register(uint32_t packet_type, PacketHandler handler);

    /**
     * @brief 路由数据包到对应的处理函数
     * @param client 客户端套接字
     * @param packet_type 数据包类型
     * @param data 数据内容
     */
    void Route(SOCKET client, uint32_t packet_type, const std::string& data);

 private:
    std::map<uint32_t, PacketHandler> routes_;  // 路由映射表
};