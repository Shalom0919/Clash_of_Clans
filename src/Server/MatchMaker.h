/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MatchMaker.h
 * File Function: 匹配系统管理
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "ClanInfo.h"

#include <mutex>
#include <vector>

/**
 * @class Matchmaker
 * @brief 管理玩家匹配队列和匹配逻辑
 */
class Matchmaker {
 public:
    /**
     * @brief 将玩家加入匹配队列
     * @param entry 匹配队列条目
     */
    void Enqueue(const MatchQueueEntry& entry);

    /**
     * @brief 从队列中移除玩家
     * @param s 玩家的套接字
     */
    void Remove(SOCKET s);

    /**
     * @brief 执行匹配逻辑
     * @return 成功匹配的玩家对列表
     */
    std::vector<std::pair<MatchQueueEntry, MatchQueueEntry>> ProcessQueue();

 private:
    std::vector<MatchQueueEntry> queue_;  // 匹配队列
    std::mutex queue_mutex_;              // 保护队列的互斥锁
};