/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     NetworkUtils.cpp
 * File Function: 网络工具函数实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "NetworkUtils.h"

#include <vector>

bool recvFixedAmount(SOCKET socket, char* buffer, int total_bytes) {
    if (buffer == nullptr || total_bytes <= 0) {
        return false;
    }

    int received = 0;
    while (received < total_bytes) {
        int ret = recv(socket, buffer + received, total_bytes - received, 0);
        if (ret <= 0) {
            // 连接关闭或发生错误
            return false;
        }
        received += ret;
    }
    return true;
}

bool sendPacket(SOCKET socket, uint32_t type, const std::string& data) {
    if (socket == INVALID_SOCKET) {
        return false;
    }

    PacketHeader header;
    header.type = type;
    header.length = static_cast<uint32_t>(data.size());

    // 发送包头
    int header_sent = send(socket, reinterpret_cast<char*>(&header),
                           sizeof(PacketHeader), 0);
    if (header_sent != sizeof(PacketHeader)) {
        return false;
    }

    // 发送包体（如果有数据）
    if (header.length > 0) {
        int body_sent = send(socket, data.c_str(),
                             static_cast<int>(header.length), 0);
        if (body_sent != static_cast<int>(header.length)) {
            return false;
        }
    }

    return true;
}

bool recvPacket(SOCKET socket, uint32_t& out_type, std::string& out_data) {
    if (socket == INVALID_SOCKET) {
        return false;
    }

    // 接收包头
    PacketHeader header;
    if (!recvFixedAmount(socket, reinterpret_cast<char*>(&header),
                         sizeof(PacketHeader))) {
        return false;
    }

    out_type = header.type;
    out_data.clear();

    // 接收包体（如果有数据）
    if (header.length > 0) {
        // 安全检查：防止过大的数据包导致内存问题
        constexpr uint32_t kMaxPacketSize = 10 * 1024 * 1024;  // 10MB
        if (header.length > kMaxPacketSize) {
            return false;
        }

        std::vector<char> buffer(header.length);
        if (!recvFixedAmount(socket, buffer.data(),
                             static_cast<int>(header.length))) {
            return false;
        }
        out_data.assign(buffer.begin(), buffer.end());
    }

    return true;
}