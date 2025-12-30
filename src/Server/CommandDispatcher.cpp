/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     CommandDispatcher.cpp
 * File Function: 命令路由分发器实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "CommandDispatcher.h"

#include <iostream>

void Router::Register(uint32_t packet_type, PacketHandler handler) {
    routes_[packet_type] = handler;
}

void Router::Route(SOCKET client, uint32_t packet_type,
                   const std::string& data) {
    auto it = routes_.find(packet_type);
    if (it != routes_.end()) {
        it->second(client, data);
    } else {
        std::cout << "[Router] 未知的数据包类型: " << packet_type << std::endl;
    }
}