/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeService.h
 * File Function: 建筑升级服务 - 统一处理升级业务逻辑
 * Author:        薛毓哲
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#pragma once

#include <string>

// 前向声明
class BaseBuilding;
class ResourceManager;
class UpgradeManager;

/**
 * @enum UpgradeError
 * @brief 升级失败原因枚举
 */
enum class UpgradeError
{
    kSuccess,            // 成功
    kMaxLevel,           // 已达最高等级
    kAlreadyUpgrading,   // 已在升级中
    kNotEnoughGold,      // 金币不足
    kNotEnoughElixir,    // 圣水不足
    kNotEnoughGem,       // 宝石不足（新增）
    kNoAvailableBuilder, // 无空闲工人
    kStartUpgradeFailed, // 启动升级失败
    kUnknownError        // 未知错误
};

/**
 * @struct UpgradeResult
 * @brief 升级结果结构体
 */
struct UpgradeResult
{
    bool success;             // 是否成功
    UpgradeError error;       // 错误码
    std::string message;      // 错误信息
    
    UpgradeResult(bool s, UpgradeError e, const std::string& msg = "")
        : success(s), error(e), message(msg) {}
    
    // 便捷构造函数
    static UpgradeResult Success() {
        return UpgradeResult(true, UpgradeError::kSuccess, "");
    }
    
    static UpgradeResult Failure(UpgradeError err, const std::string& msg) {
        return UpgradeResult(false, err, msg);
    }
};

/**
 * @class BuildingUpgradeService
 * @brief 建筑升级服务类（单例）
 * 
 * 职责：
 * - 统一处理建筑升级的业务逻辑
 * - 检查升级前置条件（等级、资源、工人）
 * - 执行升级流程（扣除资源、分配工人、启动倒计时）
 * - 提供结构化的错误信息
 * 
 * 设计原则：
 * - 单一职责：只处理升级相关的业务逻辑
 * - 依赖注入：通过构造函数注入 ResourceManager 和 UpgradeManager
 * - 错误处理：返回结构化的 UpgradeResult，而不是简单的 bool
 */
class BuildingUpgradeService
{
public:
    static BuildingUpgradeService& getInstance();
    
    /**
     * @brief 检查建筑是否可以升级（不修改任何状态）
     * @param building 要检查的建筑
     * @return 是否可以升级
     */
    bool canUpgrade(const BaseBuilding* building) const;
    
    /**
     * @brief 尝试升级建筑（执行完整的升级流程）
     * @param building 要升级的建筑
     * @return 升级结果（包含成功/失败状态、错误码、错误信息）
     */
    UpgradeResult tryUpgrade(BaseBuilding* building);
    
private:
    BuildingUpgradeService();
    BuildingUpgradeService(const BuildingUpgradeService&) = delete;
    BuildingUpgradeService& operator=(const BuildingUpgradeService&) = delete;
    
    // 检查各个前置条件
    bool checkLevel(const BaseBuilding* building) const;
    bool checkUpgrading(const BaseBuilding* building) const;
    bool checkResource(const BaseBuilding* building) const;
    bool checkBuilder(const BaseBuilding* building) const;
};
