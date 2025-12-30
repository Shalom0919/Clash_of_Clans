/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GameDataRepository.h
 * File Function: 游戏数据存储仓库
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "Managers/GameDataModels.h"
#include "Managers/GameDataSerializer.h"
#include "cocos2d.h"

#include <string>
#include <vector>
#include <functional>

/**
 * @class GameDataRepository
 * @brief 游戏数据持久化仓库（单例）
 */
class GameDataRepository {
public:
    static GameDataRepository& getInstance() {
        static GameDataRepository instance;
        return instance;
    }

    // ==================== 游戏状态存储 ====================
    bool saveGameState(const std::string& userId, const GameStateData& state) {
        std::string filePath = getGameStatePath(userId);
        std::string json = GameDataSerializer::serializeGameState(state);
        
        bool result = cocos2d::FileUtils::getInstance()->writeStringToFile(json, filePath);
        if (result) {
            CCLOG("✅ GameDataRepository: Saved game state for %s", userId.c_str());
        } else {
            CCLOG("❌ GameDataRepository: Failed to save game state for %s", userId.c_str());
        }
        return result;
    }

    GameStateData loadGameState(const std::string& userId) {
        std::string filePath = getGameStatePath(userId);
        
        if (!cocos2d::FileUtils::getInstance()->isFileExist(filePath)) {
            CCLOG("⚠️ GameDataRepository: No game state file for %s", userId.c_str());
            return GameStateData();
        }

        std::string json = cocos2d::FileUtils::getInstance()->getStringFromFile(filePath);
        if (json.empty()) {
            CCLOG("❌ GameDataRepository: Empty game state file for %s", userId.c_str());
            return GameStateData();
        }

        CCLOG("✅ GameDataRepository: Loaded game state for %s", userId.c_str());
        return GameDataSerializer::deserializeGameState(json);
    }

    bool deleteGameState(const std::string& userId) {
        std::string filePath = getGameStatePath(userId);
        if (cocos2d::FileUtils::getInstance()->isFileExist(filePath)) {
            return cocos2d::FileUtils::getInstance()->removeFile(filePath);
        }
        return true;
    }

    // ==================== 账户列表存储 ====================
    bool saveAccountList(const std::vector<AccountData>& accounts, int activeIndex) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        doc.AddMember("activeIndex", activeIndex, alloc);

        rapidjson::Value arr(rapidjson::kArrayType);
        for (const auto& acc : accounts) {
            rapidjson::Value obj(rapidjson::kObjectType);
            
            rapidjson::Value userId, username, password, token, mapName;
            userId.SetString(acc.userId.c_str(), alloc);
            username.SetString(acc.username.c_str(), alloc);
            password.SetString(acc.password.c_str(), alloc);
            token.SetString(acc.token.c_str(), alloc);
            mapName.SetString(acc.assignedMapName.c_str(), alloc);

            obj.AddMember("userId", userId, alloc);
            obj.AddMember("username", username, alloc);
            obj.AddMember("password", password, alloc);
            obj.AddMember("token", token, alloc);
            obj.AddMember("assignedMapName", mapName, alloc);

            arr.PushBack(obj, alloc);
        }
        doc.AddMember("accounts", arr, alloc);

        std::string json = JsonSerializer::stringify(doc);
        std::string filePath = getAccountListPath();
        
        return cocos2d::FileUtils::getInstance()->writeStringToFile(json, filePath);
    }

    bool loadAccountList(std::vector<AccountData>& outAccounts, int& outActiveIndex) {
        std::string filePath = getAccountListPath();
        
        if (!cocos2d::FileUtils::getInstance()->isFileExist(filePath)) {
            outAccounts.clear();
            outActiveIndex = -1;
            return false;
        }

        std::string json = cocos2d::FileUtils::getInstance()->getStringFromFile(filePath);
        rapidjson::Document doc;
        
        if (!JsonSerializer::parse(json, doc)) {
            outAccounts.clear();
            outActiveIndex = -1;
            return false;
        }

        JsonSerializer::Reader reader(doc);
        outActiveIndex = reader.readInt("activeIndex", -1);

        outAccounts.clear();
        if (doc.HasMember("accounts") && doc["accounts"].IsArray()) {
            for (const auto& item : doc["accounts"].GetArray()) {
                JsonSerializer::Reader itemReader(item);
                AccountData acc;
                acc.userId = itemReader.readString("userId");
                acc.username = itemReader.readString("username");
                acc.password = itemReader.readString("password");
                acc.token = itemReader.readString("token");
                acc.assignedMapName = itemReader.readString("assignedMapName", "map/Map1.png");
                outAccounts.push_back(acc);
            }
        }

        return true;
    }

private:
    GameDataRepository() = default;

    std::string getGameStatePath(const std::string& userId) const {
        return cocos2d::FileUtils::getInstance()->getWritablePath() + "gamedata_" + userId + ".json";
    }

    std::string getAccountListPath() const {
        return cocos2d::FileUtils::getInstance()->getWritablePath() + "accounts.json";
    }
};