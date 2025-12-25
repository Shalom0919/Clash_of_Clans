/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleScene.cpp
 * File Function: 战斗场景
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#include "BattleScene.h"
#include "AccountManager.h"
#include "BuildingManager.h"
#include "Buildings/BaseBuilding.h"
#include "Buildings/DefenseBuilding.h"
#include "DraggableMapScene.h"
#include "GridMap.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/MusicManager.h"
#include "Managers/SocketClient.h"
#include "Managers/TroopInventory.h"
#include "ResourceManager.h"
#include "Unit/UnitTypes.h"
#include <ctime>
#include <sstream>

USING_NS_CC;
using namespace ui;

// ==================== 创建场景 ====================

Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

BattleScene* BattleScene::createWithEnemyData(const AccountGameData& enemyData)
{
    return createWithEnemyData(enemyData, "Enemy");
}

BattleScene* BattleScene::createWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId)
{
    BattleScene* scene = new (std::nothrow) BattleScene();
    if (scene && scene->initWithEnemyData(enemyData, enemyUserId))
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

BattleScene* BattleScene::createWithReplayData(const std::string& replayDataStr)
{
    BattleScene* scene = new (std::nothrow) BattleScene();
    if (scene && scene->initWithReplayData(replayDataStr))
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

BattleScene::BattleScene()
{
    _battleManager = new BattleManager();
}

BattleScene::~BattleScene()
{
    CC_SAFE_DELETE(_battleManager);
}

// ==================== 初始化 ====================

bool BattleScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    setupMap();
    setupUI();

    if (_battleUI)
    {
        _battleUI->updateStatus("错误：未加载敌方基地数据！", Color4B::RED);
    }

    return true;
}

bool BattleScene::initWithEnemyData(const AccountGameData& enemyData)
{
    return initWithEnemyData(enemyData, "Enemy");
}

bool BattleScene::initWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId)
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    setupMap();
    setupUI();
    setupTouchListeners();

    // 初始化战斗管理器
    if (_battleManager)
    {
        _battleManager->init(_mapSprite, enemyData, enemyUserId, false);

        // 设置UI更新回调
        _battleManager->setUIUpdateCallback([this]() {
            if (_battleUI && _battleManager)
            {
                // 根据战斗状态更新不同的计时器
                if (_battleManager->isInReadyPhase())
                {
                    // 准备阶段：更新准备倒计时
                    int readyTime = static_cast<int>(_battleManager->getReadyPhaseRemainingTime());
                    _battleUI->updateReadyPhaseTimer(readyTime);
                    _battleUI->updateTimer(static_cast<int>(_battleManager->getRemainingTime()));
                }
                else
                {
                    // 战斗阶段：更新战斗计时器
                    _battleUI->updateTimer(static_cast<int>(_battleManager->getRemainingTime()));
                }
                
                _battleUI->updateStars(_battleManager->getStars());
                _battleUI->updateDestruction(_battleManager->getDestructionPercent());
            }
        });

        // 设置战斗正式开始回调（首次部署单位时触发）
        _battleManager->setBattleStartCallback([this]() {
            if (_battleUI)
            {
                // 隐藏准备阶段UI
                _battleUI->showReadyPhaseUI(false);
                _battleUI->updateStatus("⚔️ 战斗开始！", Color4B::RED);
                CCLOG("⚔️ UI收到战斗开始通知，隐藏准备阶段UI");
            }
        });

        // 设置战斗结束回调
        _battleManager->setBattleEndCallback([this]() {
            if (_battleUI && _battleManager)
            {
                int trophyChange = _battleManager->getStars() * 10 - (3 - _battleManager->getStars()) * 3;
                _battleUI->showResultPanel(_battleManager->getStars(), _battleManager->getDestructionPercent(),
                                           _battleManager->getGoldLooted(), _battleManager->getElixirLooted(),
                                           trophyChange, _battleManager->isReplayMode() || _isSpectateMode);

                // PVP结束通知
                if (_isPvpMode && _isAttacker)
                {
                    CCLOG("📡 发送PVP结束通知");
                    SocketClient::getInstance().endPvp();
                }
            }
        });

        // 设置部队部署回调
        _battleManager->setTroopDeployCallback([this](UnitType type, int count) {
            if (_battleUI && _battleManager)
            {
                _battleUI->updateTroopCounts(_battleManager->getTroopCount(UnitType::kBarbarian),
                                             _battleManager->getTroopCount(UnitType::kArcher),
                                             _battleManager->getTroopCount(UnitType::kGiant),
                                             _battleManager->getTroopCount(UnitType::kGoblin),
                                             _battleManager->getTroopCount(UnitType::kWallBreaker));
            }
        });
    }

    // 加载建筑
    if (_buildingManager && !enemyData.buildings.empty())
    {
        CCLOG("🏰 加载敌方基地 %zu 个建筑...", enemyData.buildings.size());
        _buildingManager->loadBuildingsFromData(enemyData.buildings, true);

        // 传递建筑给战斗管理器
        if (_battleManager)
        {
            const auto& buildings = _buildingManager->getBuildings();
            std::vector<BaseBuilding*> buildingVec(buildings.begin(), buildings.end());
            _battleManager->setBuildings(buildingVec);
        }

        if (_battleUI)
        {
            _battleUI->updateStatus(
                StringUtils::format("攻击 %s 的村庄 (大本营 Lv.%d)", enemyUserId.c_str(), enemyData.townHallLevel),
                Color4B::GREEN);
        }
    }
    else
    {
        if (_battleUI)
            _battleUI->updateStatus("错误：无法加载敌方基地！", Color4B::RED);
        CCLOG("❌ 无法加载敌方基地：没有建筑数据");
    }

    // 播放准备音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_PREPARING);

    // 进入准备阶段
    this->scheduleOnce(
        [this](float dt) {
            if (_battleManager)
            {
                // 获取当前玩家的兵力库存
                auto troops = TroopInventory::getInstance().getAllTroops();
                _battleManager->startBattle(troops);
            }

            if (_battleUI)
            {
                // 显示准备阶段UI
                _battleUI->showReadyPhaseUI(true);
                _battleUI->updateStatus("🎯 侦察敌方基地，部署士兵开始进攻！", Color4B::YELLOW);
                _battleUI->showBattleHUD(true);
                
                // 只有非观战模式才显示部队按钮
                if (!_isSpectateMode)
                {
                    _battleUI->showTroopButtons(true);
                    
                    if (_battleManager)
                    {
                        _battleUI->updateTroopCounts(_battleManager->getTroopCount(UnitType::kBarbarian),
                                                     _battleManager->getTroopCount(UnitType::kArcher),
                                                     _battleManager->getTroopCount(UnitType::kGiant),
                                                     _battleManager->getTroopCount(UnitType::kGoblin),
                                                     _battleManager->getTroopCount(UnitType::kWallBreaker));
                    }
                }
            }
        },
        0.5f, "start_battle_delay");

    scheduleUpdate();

    return true;
}

bool BattleScene::initWithReplayData(const std::string& replayDataStr)
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    // 加载回放数据
    auto& replaySystem = ReplaySystem::getInstance();
    replaySystem.loadReplay(replayDataStr);

    std::string enemyUserId = replaySystem.getReplayEnemyUserId();
    std::string enemyJson   = replaySystem.getReplayEnemyGameDataJson();

    if (enemyJson.empty())
    {
        CCLOG("❌ 回放数据缺少敌方游戏数据！");
        return false;
    }

    AccountGameData enemyData = AccountGameData::fromJson(enemyJson);

    // 设置随机种子
    srand(replaySystem.getReplaySeed());

    setupMap();
    setupUI();
    setupTouchListeners();

    // 初始化战斗管理器
    if (_battleManager)
    {
        _battleManager->init(_mapSprite, enemyData, enemyUserId, true);

        _battleManager->setUIUpdateCallback([this]() {
            if (_battleUI && _battleManager)
            {
                _battleUI->updateTimer(static_cast<int>(_battleManager->getRemainingTime()));
                _battleUI->updateStars(_battleManager->getStars());
                _battleUI->updateDestruction(_battleManager->getDestructionPercent());
            }
        });

        _battleManager->setBattleEndCallback([this]() {
            if (_battleUI && _battleManager)
            {
                int trophyChange = _battleManager->getStars() * 10 - (3 - _battleManager->getStars()) * 3;
                _battleUI->showResultPanel(_battleManager->getStars(), _battleManager->getDestructionPercent(),
                                           _battleManager->getGoldLooted(), _battleManager->getElixirLooted(),
                                           trophyChange, true);
            }
        });
    }

    // 加载建筑
    if (_buildingManager && !enemyData.buildings.empty())
    {
        _buildingManager->loadBuildingsFromData(enemyData.buildings, true);

        if (_battleManager)
        {
            const auto&                buildings = _buildingManager->getBuildings();
            std::vector<BaseBuilding*> buildingVec(buildings.begin(), buildings.end());
            _battleManager->setBuildings(buildingVec);
        }
    }

    // 回放模式直接播放战斗音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);

    scheduleUpdate();

    // 设置回放回调
    replaySystem.setDeployUnitCallback([this](UnitType type, const Vec2& pos) {
        if (_battleManager)
            _battleManager->deployUnit(type, pos);
    });

    replaySystem.setEndBattleCallback([this]() {
        if (_battleManager)
            _battleManager->endBattle(false);
    });

    // UI设置 - 回放模式不显示准备阶段UI
    if (_battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->showReadyPhaseUI(false);  // 回放模式不显示准备阶段
        _battleUI->updateStatus("🔴 战斗回放中", Color4B::RED);
        _battleUI->setEndBattleButtonText("退出回放");
        _battleUI->setReplayMode(true);
        _battleUI->showBattleHUD(true);
    }

    // 回放模式：立即开始战斗，跳过准备阶段
    if (_battleManager)
    {
        // 计算回放所需的兵力
        std::map<UnitType, int> neededTroops;
        const auto&             replayData = replaySystem.getCurrentReplayData();
        for (const auto& evt : replayData.events)
        {
            if (evt.type == ReplayEventType::DEPLOY_UNIT)
            {
                neededTroops[static_cast<UnitType>(evt.unitType)]++;
            }
        }

        _battleManager->startBattle(neededTroops);
        // 跳过准备阶段，直接进入战斗状态
        _battleManager->skipReadyPhase();
    }

    return true;
}

// ==================== 场景设置 ====================

void BattleScene::setupMap()
{
    // 创建地图背景
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);

    // 创建地图精灵
    _mapSprite = Sprite::create("map/Map1.png");
    if (_mapSprite)
    {
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        _mapSprite->setScale(1.3f);
        this->addChild(_mapSprite, 0);

        // 创建网格
        auto mapSize = _mapSprite->getContentSize();
        _gridMap     = GridMap::create(mapSize, 55.6f);
        _gridMap->setStartPixel(Vec2(1406.0f, 2107.2f));
        _mapSprite->addChild(_gridMap, 999);

        // 创建建筑管理器
        _buildingManager = BuildingManager::create();
        this->addChild(_buildingManager);
        _buildingManager->setup(_mapSprite, _gridMap);

        updateBoundary();
    }
}

void BattleScene::disableAllBuildingsBattleMode()
{
    if (!_mapSprite)
        return;

    auto& children = _mapSprite->getChildren();
    for (auto child : children)
    {
        auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(child);
        if (defenseBuilding)
        {
            defenseBuilding->disableBattleMode();
        }
    }
}

void BattleScene::enableAllBuildingsBattleMode()
{
    if (!_buildingManager)
        return;

    auto buildingSprite = _mapSprite;
    if (!buildingSprite)
        return;

    auto& children = buildingSprite->getChildren();
    for (auto child : children)
    {
        auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(child);
        if (defenseBuilding)
        {
            defenseBuilding->enableBattleMode();
        }
    }
}

void BattleScene::setupUI()
{
    if (_buildingManager)
    {
        enableAllBuildingsBattleMode();
    }
    
    _battleUI = BattleUI::create();
    this->addChild(_battleUI, 100);

    _battleUI->setEndBattleCallback([this]() {
        if (_battleManager && (_battleManager->isReplayMode() || _isSpectateMode))
        {
            returnToMainScene();
        }
        else if (_battleManager)
        {
            _battleManager->endBattle(true);
        }
    });

    _battleUI->setReturnCallback([this]() { returnToMainScene(); });

    _battleUI->setTroopSelectionCallback([this](UnitType type) { onTroopSelected(type); });
}

void BattleScene::onTroopSelected(UnitType type)
{
    _selectedUnitType = type;
    if (_battleUI)
        _battleUI->highlightTroopButton(type);
}

// ==================== 战斗逻辑 ====================

void BattleScene::update(float dt)
{
    // 主线程处理网络回调
    SocketClient::getInstance().processCallbacks();

    // 观战模式下的历史回放
    if (_isSpectateMode && !_historyReplayed)
    {
        replaySpectateHistory();
        _historyReplayed = true;
        return;
    }
    
    // 🔧 新增：观战同步超时检查
    if (_isSpectateMode && _spectatePendingEnd)
    {
        _spectatePendingEndTimer += dt;
        
        // 超时后强制结束，防止网络丢包导致永远等待
        if (_spectatePendingEndTimer >= kSpectateEndTimeout)
        {
            CCLOG("📺 观战同步超时（%.1f秒），强制结束战斗。已接收: %zu, 预期: %zu",
                  kSpectateEndTimeout, _spectateReceivedActionCount, _spectateExpectedActionCount);
            
            _spectatePendingEnd = false;
            
            if (_battleManager)
            {
                _battleManager->endBattle(false);
            }
            return;
        }
    }

    float scaledDt = dt * _timeScale;
    if (_battleManager)
    {
        _battleManager->update(scaledDt);
    }
}

void BattleScene::toggleSpeed()
{
    if (_timeScale >= 4.0f)
    {
        _timeScale = 1.0f;
    }
    else
    {
        _timeScale *= 2.0f;
    }
}

// ==================== PVP/观战模式 ====================

void BattleScene::setPvpMode(bool isAttacker)
{
    _isPvpMode  = true;
    _isAttacker = isAttacker;

    if (_battleManager)
    {
        _battleManager->setNetworkMode(true, isAttacker);
    }

    if (!_isAttacker && _battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->updateStatus("正在防守攻击者...", Color4B::RED);
    }
}

void BattleScene::setSpectateMode(const std::string& attackerId, 
                                  const std::string& defenderId,
                                  int64_t elapsedMs,
                                  const std::vector<std::string>& history)
{
    _isSpectateMode       = true;
    _isPvpMode            = false;
    _isAttacker           = false;
    _spectateAttackerId   = attackerId;
    _spectateDefenderId   = defenderId;
    _spectateElapsedMs    = elapsedMs;
    _spectateHistory      = history;
    _historyReplayed      = false;
    _spectateHistoryIndex = 0;
    
    // 🔧 初始化去重机制
    _spectateHistoryProcessed = false;
    _processedActionSet.clear();
    _pendingRemoteActions.clear();
    
    // 🔧 初始化同步结束机制
    _spectatePendingEnd = false;
    _spectateExpectedActionCount = 0;
    _spectateReceivedActionCount = history.size();  // 历史操作计入已接收数量
    _spectatePendingEndTimer = 0.0f;

    CCLOG("📺 观战模式设置: %s vs %s, 已进行 %lldms, 历史操作 %zu 个",
          attackerId.c_str(), defenderId.c_str(), (long long)elapsedMs, history.size());

    if (_battleManager)
    {
        _battleManager->setNetworkMode(true, false);
        _battleManager->setBattleMode(BattleMode::SPECTATE);
        
        // 设置时间偏移，同步战斗进度
        if (elapsedMs > 0)
        {
            _battleManager->setTimeOffset(elapsedMs);
        }
    }

    if (_battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->setSpectateMode(true);
        _battleUI->setEndBattleButtonText("退出观战");
        _battleUI->setReplayMode(true);
        
        // 根据是否有历史操作决定显示内容
        if (history.empty() && elapsedMs == 0)
        {
            // 攻击者尚未部署，显示等待状态
            _battleUI->showSpectateWaitingStatus(attackerId);
        }
        else
        {
            // 战斗已开始
            _battleUI->showReadyPhaseUI(false);
            _battleUI->updateStatus(StringUtils::format("📺 观战中: %s vs %s", 
                                                        attackerId.c_str(), defenderId.c_str()), Color4B::ORANGE);
        }
    }
}

void BattleScene::setSpectateHistory(const std::vector<std::string>& history)
{
    _spectateHistory = history;
    _historyReplayed = false;
    _spectateHistoryIndex = 0;
    
    CCLOG("📺 设置观战历史: %zu 个操作", history.size());
}

void BattleScene::replaySpectateHistory()
{
    if (!_battleManager || _spectateHistory.empty())
    {
        CCLOG("📺 replaySpectateHistory: 无历史操作需要回放");
        // 🔧 即使没有历史，也标记为已处理完成
        _spectateHistoryProcessed = true;
        return;
    }

    CCLOG("📺 开始回放观战历史: %zu 个操作", _spectateHistory.size());

    // 隐藏等待状态
    if (_battleUI)
    {
        _battleUI->showReadyPhaseUI(false);
    }

    for (size_t i = 0; i < _spectateHistory.size(); ++i)
    {
        const auto& action = _spectateHistory[i];
        
        // 🔧 将操作添加到已处理集合（用于后续去重）
        _processedActionSet.insert(action);
        
        // 格式解析: "unitType,x,y"
        std::vector<std::string> parts;
        std::stringstream        ss(action);
        std::string              item;
        while (std::getline(ss, item, ','))
        {
            parts.push_back(item);
        }

        if (parts.size() >= 3)
        {
            try
            {
                int   type = std::stoi(parts[0]);
                float x    = std::stof(parts[1]);
                float y    = std::stof(parts[2]);

                CCLOG("📺 回放历史操作[%zu]: type=%d, pos=(%.1f,%.1f)", i, type, x, y);
                _battleManager->deployUnitRemote(static_cast<UnitType>(type), Vec2(x, y));
            }
            catch (const std::exception& e)
            {
                CCLOG("❌ 解析历史操作失败: %s (%s)", action.c_str(), e.what());
            }
        }
        else
        {
            CCLOG("⚠️ 历史操作格式错误: %s", action.c_str());
        }
    }
    
    // 🔧 标记历史回放完成
    _spectateHistoryProcessed = true;
    _spectateHistoryIndex = _spectateHistory.size();
    CCLOG("📺 历史回放完成，已处理操作数: %zu", _processedActionSet.size());
    
    // 🔧 处理在历史回放期间缓存的远程操作（只处理真正的新操作）
    if (!_pendingRemoteActions.empty())
    {
        CCLOG("📺 处理缓存的远程操作: %zu 个", _pendingRemoteActions.size());
        for (const auto& pendingAction : _pendingRemoteActions)
        {
            int unitType = std::get<0>(pendingAction);
            float x = std::get<1>(pendingAction);
            float y = std::get<2>(pendingAction);
            
            // 构造操作字符串用于去重检查
            std::ostringstream oss;
            oss << unitType << "," << x << "," << y;
            std::string actionKey = oss.str();
            
            // 检查是否已在历史中处理过
            if (_processedActionSet.find(actionKey) == _processedActionSet.end())
            {
                CCLOG("📺 执行缓存的新操作: type=%d, pos=(%.1f,%.1f)", unitType, x, y);
                _processedActionSet.insert(actionKey);
                _spectateReceivedActionCount++;
                _battleManager->deployUnitRemote(static_cast<UnitType>(unitType), Vec2(x, y));
            }
            else
            {
                CCLOG("📺 跳过缓存中的重复操作: type=%d, pos=(%.1f,%.1f)", unitType, x, y);
            }
        }
        _pendingRemoteActions.clear();
    }
    
    // 🔧 检查是否可以结束（如果在历史回放期间收到了结束信号）
    checkSpectateEndCondition();
}

void BattleScene::checkSpectateEndCondition()
{
    // 仅在观战模式且收到结束信号后检查
    if (!_isSpectateMode || !_spectatePendingEnd)
    {
        return;
    }
    
    // 检查是否所有操作都已接收完毕
    // 条件：已接收操作数 >= 预期操作数
    if (_spectateReceivedActionCount >= _spectateExpectedActionCount)
    {
        CCLOG("📺 观战同步完成: 已接收 %zu 个操作，预期 %zu 个，触发战斗结束",
              _spectateReceivedActionCount, _spectateExpectedActionCount);
        
        _spectatePendingEnd = false;  // 防止重复触发
        
        if (_battleManager)
        {
            _battleManager->endBattle(false);
        }
    }
    else
    {
        CCLOG("📺 观战同步等待中: 已接收 %zu/%zu 个操作",
              _spectateReceivedActionCount, _spectateExpectedActionCount);
              
        // 更新UI显示同步状态
        if (_battleUI)
        {
            _battleUI->updateStatus(
                StringUtils::format("📺 同步数据中... (%zu/%zu)", 
                                    _spectateReceivedActionCount, 
                                    _spectateExpectedActionCount), 
                Color4B::YELLOW);
        }
    }
}

// ==================== 场景生命周期 ====================

void BattleScene::onEnter()
{
    Scene::onEnter();

    if (_isPvpMode || _isSpectateMode)
    {
        auto& client = SocketClient::getInstance();

        // 接收远程操作
        client.setOnPvpAction([this](int unitType, float x, float y) {
            if (_battleManager)
            {
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, unitType, x, y]() {
                    if (!_battleManager)
                        return;
                        
                    // 观战模式下，使用操作内容去重
                    if (_isSpectateMode)
                    {
                        // 🔧 修复：如果历史回放尚未完成，缓存此操作
                        if (!_spectateHistoryProcessed)
                        {
                            CCLOG("📺 历史回放未完成，缓存远程操作: type=%d, pos=(%.1f,%.1f)", 
                                  unitType, x, y);
                            _pendingRemoteActions.push_back(std::make_tuple(unitType, x, y));
                            return;
                        }
                        
                        // 🔧 构造操作字符串用于去重检查
                        std::ostringstream oss;
                        oss << unitType << "," << x << "," << y;
                        std::string actionKey = oss.str();
                        
                        // 检查是否已处理过此操作
                        if (_processedActionSet.find(actionKey) != _processedActionSet.end())
                        {
                            CCLOG("📺 跳过已处理的重复操作: type=%d, pos=(%.1f,%.1f)", 
                                  unitType, x, y);
                            return;
                        }
                        
                        // 🔧 记录此操作为已处理，并增加已接收计数
                        _processedActionSet.insert(actionKey);
                        _spectateReceivedActionCount++;
                        
                        // 首次收到新的远程操作，隐藏等待状态
                        if (_battleUI)
                        {
                            _battleUI->showReadyPhaseUI(false);
                            _battleUI->updateStatus(StringUtils::format("📺 观战中: %s vs %s", 
                                _spectateAttackerId.c_str(), _spectateDefenderId.c_str()), Color4B::ORANGE);
                        }
                        
                        CCLOG("📥 收到远程部署（新操作 %zu/%zu）: type=%d, pos=(%.1f,%.1f)", 
                              _spectateReceivedActionCount, _spectateExpectedActionCount,
                              unitType, x, y);
                    }
                    else
                    {
                        CCLOG("📥 收到远程部署: type=%d, pos=(%.1f,%.1f)", unitType, x, y);
                    }
                    
                    _battleManager->deployUnitRemote((UnitType)unitType, Vec2(x, y));
                    
                    // 🔧 检查是否所有操作都已接收完毕
                    checkSpectateEndCondition();
                });
            }
        });

        // 接收结束通知
        client.setOnPvpEnd([this](const std::string& result) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, result]() {
                CCLOG("📥 收到战斗结束通知: %s", result.c_str());
                
                if (_isSpectateMode)
                {
                    // 🔧 修复：解析结束消息，获取总操作数
                    // 格式: "BATTLE_ENDED|totalActionCount"
                    size_t expectedCount = 0;
                    size_t separatorPos = result.find('|');
                    if (separatorPos != std::string::npos)
                    {
                        try
                        {
                            expectedCount = std::stoul(result.substr(separatorPos + 1));
                        }
                        catch (const std::exception& e)
                        {
                            CCLOG("⚠️ 解析总操作数失败: %s", e.what());
                        }
                    }
                    
                    _spectateExpectedActionCount = expectedCount;
                    _spectatePendingEnd = true;
                    
                    CCLOG("📺 观战结束信号: 预期操作数=%zu, 已接收=%zu", 
                          _spectateExpectedActionCount, _spectateReceivedActionCount);
                    
                    // 🔧 检查是否可以立即结束（所有操作都已接收）
                    checkSpectateEndCondition();
                }
                else if (!_isAttacker)
                {
                    if (_battleManager)
                    {
                        _battleManager->endBattle(false);
                    }
                }
            });
        });

        // 发送本地操作（仅攻击方，非观战模式）
        if (_battleManager && _isAttacker && !_isSpectateMode)
        {
            _battleManager->setNetworkDeployCallback([this](UnitType type, const Vec2& pos) {
                CCLOG("📤 发送远程部署: type=%d, pos=(%.1f,%.1f)", (int)type, pos.x, pos.y);
                SocketClient::getInstance().sendPvpAction((int)type, pos.x, pos.y);
            });
        }
    }
}

void BattleScene::onExit()
{
    // 清理网络回调
    if (_isPvpMode || _isSpectateMode)
    {
        auto& client = SocketClient::getInstance();
        client.setOnPvpAction(nullptr);
        client.setOnPvpEnd(nullptr);

        if (_battleManager)
        {
            _battleManager->setNetworkDeployCallback(nullptr);
        }

        // 攻击方在退出时发送结束通知
        if (_isAttacker && !_isSpectateMode)
        {
            client.endPvp();
        }
    }

    Scene::onExit();
}

// ==================== 触摸监听器设置 ====================

void BattleScene::setupTouchListeners()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (_battleManager && _battleManager->getState() == BattleManager::BattleState::FINISHED)
            return false;

        _activeTouches[touch->getID()] = touch->getLocation();

        _lastTouchPos = touch->getLocation();
        _isDragging   = false;
        return true;
    };

    touchListener->onTouchMoved = [this](Touch* touch, Event* event) {
        if (_activeTouches.find(touch->getID()) != _activeTouches.end())
        {
            _activeTouches[touch->getID()] = touch->getLocation();
        }

        // 多点触控缩放
        if (_activeTouches.size() >= 2)
        {
            _isPinching = true;
            _isDragging = false;

            auto it = _activeTouches.begin();
            Vec2 p1 = it->second;
            it++;
            Vec2 p2 = it->second;

            float currentDist = p1.distance(p2);

            if (_prevPinchDistance <= 0.0f)
            {
                _prevPinchDistance = currentDist;
            }
            else
            {
                if (currentDist > 10.0f && _mapSprite)
                {
                    float zoomFactor = currentDist / _prevPinchDistance;
                    zoomFactor = std::max(0.9f, std::min(zoomFactor, 1.1f));

                    float newScale = _mapSprite->getScale() * zoomFactor;
                    newScale       = std::max(0.9f, std::min(newScale, 2.0f));
                    _mapSprite->setScale(newScale);

                    updateBoundary();
                    ensureMapInBoundary();

                    _prevPinchDistance = currentDist;
                }
            }
            return;
        }
        else
        {
            _prevPinchDistance = 0.0f;
        }

        Vec2 currentPos = touch->getLocation();
        Vec2 delta      = currentPos - touch->getPreviousLocation();

        if (currentPos.distance(_lastTouchPos) > 10.0f)
        {
            _isDragging = true;
        }

        if (_mapSprite && _isDragging && !_isPinching)
        {
            Vec2 newPos = _mapSprite->getPosition() + delta;
            _mapSprite->setPosition(newPos);
            ensureMapInBoundary();
        }
    };

    touchListener->onTouchEnded = [this](Touch* touch, Event* event) {
        _activeTouches.erase(touch->getID());
        if (_activeTouches.size() < 2)
        {
            _prevPinchDistance = 0.0f;
        }

        if (_isPinching)
        {
            if (_activeTouches.empty())
            {
                _isPinching = false;
            }
            return;
        }

        // 观战模式不允许部署
        if (_isSpectateMode)
        {
            _isDragging = false;
            return;
        }

        if (!_isDragging && _battleManager &&
            (_battleManager->getState() == BattleManager::BattleState::READY ||
             _battleManager->getState() == BattleManager::BattleState::FIGHTING))
        {
            Vec2 touchPos    = touch->getLocation();
            Vec2 mapLocalPos = _mapSprite->convertToNodeSpace(touchPos);
            _battleManager->deployUnit(_selectedUnitType, mapLocalPos);
        }
        _isDragging = false;
    };

    touchListener->onTouchCancelled = [this](Touch* touch, Event* event) {
        _activeTouches.erase(touch->getID());
        if (_activeTouches.size() < 2)
        {
            _prevPinchDistance = 0.0f;
            if (_activeTouches.empty())
                _isPinching = false;
        }
        _isDragging = false;
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    auto mouseListener           = EventListenerMouse::create();
    mouseListener->onMouseScroll = [this](Event* event) {
        EventMouse* mouseEvent = static_cast<EventMouse*>(event);
        float       scrollY    = mouseEvent->getScrollY();

        if (_mapSprite)
        {
            float zoomFactor = scrollY < 0 ? 1.1f : 0.9f;
            float newScale   = _mapSprite->getScale() * zoomFactor;
            newScale         = std::max(0.9f, std::min(newScale, 2.0f));
            _mapSprite->setScale(newScale);

            updateBoundary();
            ensureMapInBoundary();
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void BattleScene::returnToMainScene()
{
    CCLOG("🚪 返回主场景");
    
    MusicManager::getInstance().stopMusic();
    disableAllBuildingsBattleMode();
    
    auto director = Director::getInstance();
    
    if (_isPushedScene)
    {
        director->popScene();
        
        director->getScheduler()->performFunctionInCocosThread([]() {
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("scene_resume");
        });
    }
    else
    {
        CCLOG("⚠️ 使用 replaceScene 返回主场景");
        
        auto mainScene = createDraggableMapScene();
        if (mainScene)
        {
            director->replaceScene(TransitionFade::create(0.3f, mainScene));
        }
    }
}

void BattleScene::updateBoundary()
{
    if (!_mapSprite)
        return;
    auto  mapSize = _mapSprite->getContentSize() * _mapSprite->getScale();
    float minX    = _visibleSize.width - mapSize.width / 2;
    float maxX    = mapSize.width / 2;
    float minY    = _visibleSize.height - mapSize.height / 2;
    float maxY    = mapSize.height / 2;
    if (mapSize.width <= _visibleSize.width)
        minX = maxX = _visibleSize.width / 2;
    if (mapSize.height <= _visibleSize.height)
        minY = maxY = _visibleSize.height / 2;
    _mapBoundary = Rect(minX, minY, maxX - minX, maxY - minY);
}

void BattleScene::ensureMapInBoundary()
{
    if (!_mapSprite)
        return;
    Vec2 currentPos = _mapSprite->getPosition();
    Vec2 newPos     = currentPos;
    if (currentPos.x < _mapBoundary.getMinX())
        newPos.x = _mapBoundary.getMinX();
    else if (currentPos.x > _mapBoundary.getMaxX())
        newPos.x = _mapBoundary.getMaxX();
    if (currentPos.y < _mapBoundary.getMinY())
        newPos.y = _mapBoundary.getMinY();
    else if (currentPos.y > _mapBoundary.getMaxY())
        newPos.y = _mapBoundary.getMaxY();
    if (newPos != currentPos)
        _mapSprite->setPosition(newPos);
}