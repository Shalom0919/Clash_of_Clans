/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     NetworkUtils.h
 * File Function: 网络工具函数声明
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "Protocol.h"

#include <WinSock2.h>

#include <cstdint>
#include <string>

/**
 * @brief 发送数据包到指定套接字
 * @param socket 目标套接字
 * @param type 数据包类型
 * @param data 数据内容
 * @return 发送成功返回true，失败返回false
 */
bool sendPacket(SOCKET socket, uint32_t type, const std::string& data);

/**
 * @brief 从套接字接收数据包
 * @param socket 源套接字
 * @param out_type 输出参数，接收到的数据包类型
 * @param out_data 输出参数，接收到的数据内容
 * @return 接收成功返回true，失败返回false
 */
bool recvPacket(SOCKET socket, uint32_t& out_type, std::string& out_data);

/**
 * @brief 从套接字接收固定数量的字节
 * @param socket 源套接字
 * @param buffer 接收缓冲区
 * @param total_bytes 需要接收的字节数
 * @return 接收成功返回true，失败返回false
 */
bool recvFixedAmount(SOCKET socket, char* buffer, int total_bytes);