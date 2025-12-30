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

    // ==================== 向后兼容引用（用于旧代码）====================
    // 这些引用提供对旧代码的兼容性，使 info.userId 等价于 info.account.userId

    std::string& userId          = account.userId;
    std::string& username        = account.username;
    std::string& password        = account.password;
    std::string& token           = account.token;
    std::string& assignedMapName = account.assignedMapName;

    // gameData 别名（向后兼容）
    GameStateData& gameData = gameState;

    // 默认构造函数
    AccountInfo()
        : userId(account.userId), username(account.username), password(account.password), token(account.token),
          assignedMapName(account.assignedMapName), gameData(gameState)
    {}

    // 拷贝构造函数
    AccountInfo(const AccountInfo& other)
        : account(other.account), gameState(other.gameState), userId(account.userId), username(account.username),
          password(account.password), token(account.token), assignedMapName(account.assignedMapName),
          gameData(gameState)
    {}

    // 拷贝赋值运算符
    AccountInfo& operator=(const AccountInfo& other)
    {
        if (this != &other)
        {
            account   = other.account;
            gameState = other.gameState;
        }
        return *this;
    }

    // 移动构造函数
    AccountInfo(AccountInfo&& other) noexcept
        : account(std::move(other.account)), gameState(std::move(other.gameState)), userId(account.userId),
          username(account.username), password(account.password), token(account.token),
          assignedMapName(account.assignedMapName), gameData(gameState)
    {}

    // 移动赋值运算符
    AccountInfo& operator=(AccountInfo&& other) noexcept
    {
        if (this != &other)
        {
            account   = std::move(other.account);
            gameState = std::move(other.gameState);
        }
        return *this;
    }
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
    void                            upsertAccount(const AccountInfo& info); // 向后兼容重载
    const std::vector<AccountInfo>& listAccounts() const;
    void                            signOut();
    bool                            verifyPassword(const std::string& userId, const std::string& password) const;
    bool                            deleteAccount(const std::string& userId);

    // ==================== 游戏状态管理（新 API）====================
    void          updateGameState(const GameStateData& state);
    GameStateData getCurrentGameState() const;
    bool          saveCurrentGameState();
    bool          loadGameStateForUser(const std::string& userId);
    GameStateData getPlayerGameState(const std::string& userId) const;

    std::string exportGameStateJson() const;
    bool        importGameStateJson(const std::string& userId, const std::string& jsonData);

    void save();

    // ==================== 向后兼容方法（旧 API）====================
    GameStateData getCurrentGameData() const { return getCurrentGameState(); }
    void          updateGameData(const GameStateData& state) { updateGameState(state); }
    GameStateData getPlayerGameData(const std::string& userId) const { return getPlayerGameState(userId); }
    bool          saveGameStateToFile() { return saveCurrentGameState(); }

private:
    AccountManager()                                 = default;
    AccountManager(const AccountManager&)            = delete;
    AccountManager& operator=(const AccountManager&) = delete;

    std::vector<AccountInfo> _accounts;
    int                      _activeIndex = -1;

    void loadFromStorage();
};