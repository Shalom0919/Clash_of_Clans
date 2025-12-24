/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleScene.cpp
 * File Function: 战斗场景
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "BattleScene.h"
#include "AccountManager.h"
#include "BuildingManager.h"
#include "Buildings/BaseBuilding.h"
#include "Buildings/DefenseBuilding.h"
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
                _battleUI->updateTimer(static_cast<int>(_battleManager->getRemainingTime()));
                _battleUI->updateStars(_battleManager->getStars());
                _battleUI->updateDestruction(_battleManager->getDestructionPercent());
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

    // 延迟开始战斗
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
                _battleUI->updateStatus("部署你的士兵进行攻击！", Color4B::YELLOW);
                _battleUI->showBattleHUD(true);
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
        },
        1.0f, "start_battle_delay");

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

    // UI设置
    if (_battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->updateStatus("🔴 战斗回放中", Color4B::RED);
        _battleUI->setEndBattleButtonText("退出回放");
        _battleUI->setReplayMode(true);
        _battleUI->showBattleHUD(true);
    }

    // 立即开始战斗
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

    CCLOG("📺 观战模式设置: %s vs %s, 已进行 %lldms, 历史操作 %zu 个",
          attackerId.c_str(), defenderId.c_str(), (long long)elapsedMs, history.size());

    if (_battleManager)
    {
        _battleManager->setNetworkMode(true, false);
    }

    if (_battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->updateStatus(StringUtils::format("📺 观战中: %s vs %s", 
                                                    attackerId.c_str(), defenderId.c_str()), Color4B::ORANGE);
        _battleUI->setEndBattleButtonText("退出观战");
        _battleUI->setReplayMode(true);
    }
}

void BattleScene::setSpectateHistory(const std::vector<std::string>& history)
{
    _spectateHistory = history;
    _historyReplayed = false;
    
    CCLOG("📺 设置观战历史: %zu 个操作", history.size());
}

void BattleScene::replaySpectateHistory()
{
    if (!_battleManager || _spectateHistory.empty())
        return;

    CCLOG("📺 回放观战历史: %zu 个操作", _spectateHistory.size());

    for (const auto& action : _spectateHistory)
    {
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

                CCLOG("📺 回放操作: type=%d, pos=(%.1f,%.1f)", type, x, y);
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
                    if (_battleManager)
                    {
                        CCLOG("📥 收到远程部署: type=%d, pos=(%.1f,%.1f)", unitType, x, y);
                        _battleManager->deployUnitRemote((UnitType)unitType, Vec2(x, y));
                    }
                });
            }
        });

        // 接收结束通知
        client.setOnPvpEnd([this](const std::string& result) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, result]() {
                CCLOG("📥 收到战斗结束通知: %s", result.c_str());
                
                if (_isSpectateMode)
                {
                    // 观战模式下收到结束通知，显示结果面板
                    if (_battleManager)
                    {
                        _battleManager->endBattle(false);
                    }
                }
                else if (!_isAttacker)
                {
                    // 防守方收到结束通知
                    if (_battleManager)
                    {
                        _battleManager->endBattle(false);
                    }
                }
            });
        });

        // 发送本地操作（仅攻击方）
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
    
    // 使用 popScene 而不是 end()
    Director::getInstance()->popScene();
    
    // 通知主场景恢复
    Director::getInstance()->getScheduler()->performFunctionInCocosThread([]() {
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("scene_resume");
    });
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