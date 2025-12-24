/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AccountManager.h
 * File Function: 账户管理器（重构版）
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "GameDataModels.h"
#include "GameDataRepository.h"

#include <string>
#include <vector>

/**
 * @struct AccountInfo
 * @brief 完整账户信息（内存中）
 */
struct AccountInfo
{
    AccountData   account;
    GameStateData gameState;
};

/**
 * @class AccountManager
 * @brief 账户管理器（单例）
 */
class AccountManager
{
public:
    static AccountManager& getInstance();

    bool initialize();

    const AccountInfo*              getCurrentAccount() const;
    bool                            switchAccount(const std::string& userId, bool silent = false);
    void                            upsertAccount(const AccountData& acc);
    const std::vector<AccountInfo>& listAccounts() const;
    void                            signOut();
    bool                            verifyPassword(const std::string& userId, const std::string& password) const;
    bool                            deleteAccount(const std::string& userId);

    void          updateGameState(const GameStateData& state);
    GameStateData getCurrentGameState() const;
    bool          saveCurrentGameState();
    bool          loadGameStateForUser(const std::string& userId);
    GameStateData getPlayerGameState(const std::string& userId) const;

    std::string exportGameStateJson() const;
    bool        importGameStateJson(const std::string& userId, const std::string& jsonData);

    void save();

private:
    AccountManager()                                 = default;
    AccountManager(const AccountManager&)            = delete;
    AccountManager& operator=(const AccountManager&) = delete;

    std::vector<AccountInfo> _accounts;
    int                      _activeIndex = -1;

    void loadFromStorage();
};