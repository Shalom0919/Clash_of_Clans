/**
 * @file DraggableMapScene.cpp
 * @brief 主场景实现 - 重构后的精简版本
 */

#include "AccountManager.h"
#include "BaseBuilding.h"
#include "BattleScene.h"
#include "BuildingCapacityManager.h"
#include "BuildingData.h"
#include "BuildingManager.h"
#include "Buildings/ArmyBuilding.h"
#include "Buildings/ResourceBuilding.h"
#include "BuildingUpgradeUI.h"
#include "DraggableMapScene.h"
#include "HUDLayer.h"
#include "InputController.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/ResourceCollectionManager.h"
#include "Managers/UpgradeManager.h"
#include "MapController.h"
#include "ResourceManager.h"
#include "SceneUIController.h"
#include "ShopLayer.h"
#include "SocketClient.h"
#include "UI/ArmySelectionUI.h"
#include "UI/PlayerListLayer.h"
#include "Unit/unit.h"
#include "BuildingCapacityManager.h"
#include "ui/CocosGUI.h"

USING_NS_CC;
using namespace ui;

Scene* DraggableMapScene::createScene()
{
    return DraggableMapScene::create();
}

bool DraggableMapScene::init()
{
    if (!Scene::init())
    {
        return false;
    }
    // 1. 获取单例
    auto* capacityMgr = &BuildingCapacityManager::getInstance();
    if (capacityMgr->getParent())
    {
        capacityMgr->removeFromParent();
    }
    this->addChild(capacityMgr, 0);

    ResourceCollectionManager* mgr = ResourceCollectionManager::getInstance();

    // 🔴 关键步骤：将单例 Node 添加到场景中（只需一次），这样它的触摸监听和 update 才会工作。
    if (mgr->getParent())
    {
        mgr->removeFromParent();
    }
    this->addChild(mgr, 0); // 较低 Z-order，确保不遮挡UI
    _visibleSize = Director::getInstance()->getVisibleSize();

    initializeManagers();
    setupCallbacks();
    setupUpgradeManagerCallbacks(); // ✅ 添加升级管理器回调设置

    connectToServer();
    setupNetworkCallbacks();

    scheduleUpdate();
    
    // ✅ 监听场景重新激活事件（从其他场景返回时）
    auto listener = EventListenerCustom::create("scene_resume", [this](EventCustom* event) {
        this->onSceneResume();
    });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    // 在创建 HUDLayer 之后添加：
    auto hudLayer = HUDLayer::create();
    this->addChild(hudLayer, 100); // 假设这是在场景里

    // 绑定回调：当 UpgradeManager 通知工人变化时，刷新 HUD
    UpgradeManager::getInstance()->setOnAvailableBuilderChanged(
        [hudLayer](int available) { hudLayer->updateDisplay(); });

    // 🆕 加载防守日志
    DefenseLogSystem::getInstance().load();

    // 🆕 检查是否有未查看的防守日志
    if (DefenseLogSystem::getInstance().hasUnviewedLogs())
    {
        this->scheduleOnce(
            [this](float dt) {
                // 延迟1秒后显示防守日志
                DefenseLogSystem::getInstance().showDefenseLogUI();
            },
            1.0f, "show_defense_log");
    }

    
    // 延迟加载游戏状态
    this->scheduleOnce([this](float dt) { loadGameState(); }, 0.1f, "load_game_state");

    return true;
}

void DraggableMapScene::initializeManagers()
{
    // ==================== 获取当前账号分配的地图 ====================
    std::string assignedMap    = "map/Map1.png"; // 默认地图
    auto&       accMgr         = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (currentAccount && !currentAccount->assignedMapName.empty())
    {
        assignedMap = currentAccount->assignedMapName;
        CCLOG("✅ Loading assigned map for account %s: %s", currentAccount->username.c_str(), assignedMap.c_str());
    }

    // ==================== 地图控制器 ====================
    _mapController = MapController::create();
    this->addChild(_mapController, 0);
    _mapController->setMapNames({assignedMap}); // 只设置当前账号的地图
    _mapController->loadMap(assignedMap);

    // ==================== 建筑管理器 ====================
    _buildingManager = BuildingManager::create();
    this->addChild(_buildingManager);
    _buildingManager->setup(_mapController->getMapSprite(), _mapController->getGridMap());

    // ==================== UI 控制器 ====================
    _uiController = SceneUIController::create();
    this->addChild(_uiController, 100);

    initBuildingData();

    // ==================== 输入控制器 ====================
    _inputController = InputController::create();
    this->addChild(_inputController);

    // ==================== HUD ====================
    _hudLayer = HUDLayer::create();
    this->addChild(_hudLayer, 100);

    // ==================== ✅ 资源收集管理器 ====================
}

void DraggableMapScene::setupCallbacks()
{
    // ==================== 输入回调 ====================
    _inputController->setOnTouchBegan([this](Touch* touch, Event* event) { return onTouchBegan(touch, event); });

    _inputController->setOnTouchMoved([this](Touch* touch, Event* event) { onTouchMoved(touch, event); });

    _inputController->setOnTouchEnded([this](Touch* touch, Event* event) { onTouchEnded(touch, event); });

    _inputController->setOnMouseScroll([this](float scrollY, Vec2 mousePos) { onMouseScroll(scrollY, mousePos); });

    _inputController->setOnKeyPressed([this](EventKeyboard::KeyCode keyCode) { onKeyPressed(keyCode); });

    // ==================== UI 回调 ====================
    _uiController->setOnShopClicked([this]() { onShopClicked(); });

    _uiController->setOnAttackClicked([this]() { onAttackClicked(); });

    _uiController->setOnClanClicked([this]() { onClanClicked(); });

    _uiController->setOnBuildingSelected([this](const BuildingData& data) { onBuildingSelected(data); });

    _uiController->setOnConfirmBuilding([this]() { onConfirmBuilding(); });

    _uiController->setOnCancelBuilding([this]() { onCancelBuilding(); });

    _uiController->setOnAccountSwitched([this]() { onAccountSwitched(); });

    _uiController->setOnLogout([this]() { onLogout(); });

    _uiController->setOnMapChanged([this](const std::string& newMap) { onMapChanged(newMap); });

    // ==================== 建筑管理器回调 ====================
    _buildingManager->setOnBuildingPlaced([this](BaseBuilding* building) { onBuildingPlaced(building); });

    _buildingManager->setOnBuildingClicked([this](BaseBuilding* building) { onBuildingClicked(building); });

    _buildingManager->setOnHint([this](const std::string& hint) { onBuildingHint(hint); });
}

// ==================== ✅ 新增：升级管理器回调设置 ====================
void DraggableMapScene::setupUpgradeManagerCallbacks()
{
    // ✅ 监听升级管理器的工人数量变化
    auto* upgradeMgr = UpgradeManager::getInstance();
    upgradeMgr->setOnAvailableBuilderChanged([this](int availableBuilders) {
        // 当工人数量变化时，强制更新HUD显示
        if (_hudLayer)
        {
            _hudLayer->updateDisplay();
        }
        CCLOG("👷 工人数量已更新：可用=%d", availableBuilders);
    });
}

void DraggableMapScene::initBuildingData()
{
    std::vector<BuildingData> buildingList;

    buildingList.push_back(
        BuildingData("TownHall", "buildings/BaseCamp/town-hall-1.png", Size(5, 5), 1.0f, 0, 0, ResourceType::kGold));
    buildingList.push_back(BuildingData("ArcherTower", "buildings/ArcherTower/Archer_Tower1.png", Size(3, 3), 0.8f,
                                        1000, 60, ResourceType::kGold));
    buildingList.push_back(
        BuildingData("Cannon", "buildings/Cannon_Static/Cannon1.png", Size(3, 3), 0.8f, 500, 30, ResourceType::kGold));
    buildingList.push_back(
        BuildingData("Wall", "buildings/Wall/Wall1.png", Size(1, 1), 0.6f, 50, 0, ResourceType::kGold));
    buildingList.push_back(BuildingData("Barracks", "buildings/Barracks/Barracks1.png", Size(4, 4), 0.6f, 1500, 120,
                                        ResourceType::kElixir));
    buildingList.push_back(
        BuildingData("ArmyCamp", "buildings/ArmyCamp/Army_Camp1.png", Size(4, 4), 0.8f, 250, 0, ResourceType::kElixir));
    buildingList.push_back(
        BuildingData("GoldMine", "buildings/GoldMine/Gold_Mine1.png", Size(3, 3), 0.8f, 800, 45, ResourceType::kGold));
    buildingList.push_back(BuildingData("ElixirCollector", "buildings/ElixirCollector/Elixir_Collector1.png",
                                        Size(3, 3), 0.8f, 750, 40, ResourceType::kGold));
    buildingList.push_back(BuildingData("GoldStorage", "buildings/GoldStorage/Gold_Storage1.png", Size(3, 3), 0.8f,
                                        1000, 30, ResourceType::kGold));
    buildingList.push_back(BuildingData("ElixirStorage", "buildings/ElixirStorage/Elixir_Storage1.png", Size(3, 3),
                                        0.8f, 1000, 30, ResourceType::kGold));
    buildingList.push_back(BuildingData("BuilderHut", "buildings/BuildersHut/Builders_Hut1.png", Size(2, 2), 0.7f, 0, 0,
                                        ResourceType::kGold));

    _uiController->setBuildingList(buildingList);
}

void DraggableMapScene::loadGameState()
{
    if (_buildingManager)
    {
        _buildingManager->loadCurrentAccountState();
        CCLOG("? Game state loaded");
    }
}

// ==================== 输入处理 ====================

bool DraggableMapScene::onTouchBegan(Touch* touch, Event* event)
{
    Vec2 touchPos = touch->getLocation();

    // 【优先级0】✅ 资源收集优先处理
    if (_collectionMgr && _collectionMgr->handleTouch(touchPos))
    {
        CCLOG("✅ 资源收集：触摸已处理");
        return true;
    }

    // 【优先级1】升级UI
    if (_currentUpgradeUI && _currentUpgradeUI->isVisible())
    {
        Vec2 localPos = _currentUpgradeUI->convertTouchToNodeSpace(touch);
        Rect bbox     = _currentUpgradeUI->getBoundingBox();
        bbox.origin   = Vec2::ZERO;

        if (bbox.containsPoint(localPos))
        {
            return true; // UI 内部处理
        }
        else
        {
            hideUpgradeUI();
            return false;
        }
    }

    // 【优先级2】建筑移动模式（高于建造模式）
    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        // 建筑移动模式下，不处理场景触摸
        return false;
    }

    // 【优先级3】建筑建造模式
    if (_buildingManager && _buildingManager->isInBuildingMode())
    {
        if (!_buildingManager->isDraggingBuilding() && !_buildingManager->isWaitingConfirm())
        {
            _buildingManager->onTouchBegan(touchPos);
            return true;
        }
    }

    // 【优先级4】建筑点击检测（新增）
    if (_buildingManager)
    {
        BaseBuilding* clickedBuilding = _buildingManager->getBuildingAtPosition(touchPos);
        if (clickedBuilding)
        {
            // 记录点击的建筑和触摸起点，用于后续判断是点击还是拖动
            _clickedBuilding = clickedBuilding;
            _touchBeganPos   = touchPos;
            _touchBeganTime  = Director::getInstance()->getTotalFrames() / 60.0f;
            _hasMoved        = false;
            return true;
        }
    }

    // 【优先级5】地图操作（最低）
    _clickedBuilding = nullptr;
    return true;
}

void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    Vec2 currentPos  = touch->getLocation();
    Vec2 previousPos = touch->getPreviousLocation();
    Vec2 delta       = currentPos - previousPos;

    // 检测是否移动了足够距离
    if (_clickedBuilding)
    {
        float distance = currentPos.distance(_touchBeganPos);

        // 如果移动距离超过10像素，标记为已移动
        if (distance > 10.0f)
        {
            _hasMoved = true;

            // 如果移动超过30像素，进入建筑移动模式
            if (distance > 30.0f && _buildingManager && !_buildingManager->isMovingBuilding() &&
                !_buildingManager->isInBuildingMode())
            {
                _buildingManager->startMovingBuilding(_clickedBuilding);
                _clickedBuilding = nullptr; // 清空，后续由 BuildingManager 处理
                return;
            }
        }
    }

    // 【优先级1】建筑移动模式
    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        _buildingManager->onBuildingTouchMoved(currentPos);
        return;
    }

    // 【优先级2】建筑建造模式
    if (_buildingManager && _buildingManager->isInBuildingMode() && _buildingManager->isDraggingBuilding())
    {
        _buildingManager->onTouchMoved(currentPos);
        return;
    }

    // 【优先级3】地图平移
    if (!_clickedBuilding || _hasMoved)
    {
        _mapController->moveMap(delta);
    }
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event)
{
    cocos2d::Vec2 worldPos = touch->getLocation();

    // 1. 检查是否在收集资源
    if (ResourceCollectionManager::getInstance()->handleTouch(worldPos))
    {
        // 如果处理了收集事件，则停止进一步处理（不移动地图，不选择建筑）
        return;
    }
    Vec2 touchPos = touch->getLocation();

    // 【优先级1】建筑移动模式
    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        // 建筑移动由 BuildingManager 内部处理
        BaseBuilding* movingBuilding = _buildingManager->getMovingBuilding();
        if (movingBuilding)
        {
            _buildingManager->onBuildingTouchEnded(touchPos, movingBuilding);
        }
        _clickedBuilding = nullptr;
        _hasMoved        = false;
        return;
    }

    // 【优先级2】建筑建造模式
    if (_buildingManager && _buildingManager->isInBuildingMode() && _buildingManager->isDraggingBuilding())
    {
        _buildingManager->onTouchEnded(touchPos);

        if (_buildingManager->isWaitingConfirm())
        {
            Vec2 worldPos = _buildingManager->getPendingBuildingWorldPos();
            _uiController->showConfirmButtons(worldPos);
        }
        _clickedBuilding = nullptr;
        _hasMoved        = false;
        return;
    }

    // 【优先级3】建筑点击（单击）
    if (_clickedBuilding && !_hasMoved && !_buildingManager->isInBuildingMode())
    {
        // 触发建筑点击回调
        onBuildingClicked(_clickedBuilding);
        _clickedBuilding = nullptr;
        _hasMoved        = false;
        return;
    }

    // 重置状态
    _clickedBuilding = nullptr;
    _hasMoved        = false;
}

void DraggableMapScene::onMouseScroll(float scrollY, Vec2 mousePos)
{
    float zoomFactor = scrollY > 0 ? 0.9f : 1.1f;
    _mapController->zoomMap(zoomFactor, mousePos);
}

void DraggableMapScene::onKeyPressed(EventKeyboard::KeyCode keyCode)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        if (_buildingManager && _buildingManager->isInBuildingMode())
        {
            _buildingManager->cancelPlacing();
            _uiController->hideConfirmButtons();
        }
    }
}

// ==================== UI 回调 ====================

void DraggableMapScene::onShopClicked()
{
    if (_buildingManager && _buildingManager->isInBuildingMode())
        return;

    auto shop = ShopLayer::create();
    this->addChild(shop, 200);
    shop->show();
}

void DraggableMapScene::onAttackClicked()
{
    // 保存当前状态
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved current base before attacking");
    }

    // 🆕 第一步：显示军队选择UI
    auto armyUI = ArmySelectionUI::create();
    if (!armyUI)

    auto& accMgr = AccountManager::getInstance();
    const auto& allAccounts = accMgr.listAccounts();
    const auto* currentAccount = accMgr.getCurrentAccount();

    if (!currentAccount)
    if (!currentAccount)
    if (!currentAccount)
    {
        _uiController->showHint("创建军队选择UI失败！");

    this->addChild(armyUI, 200);

    // 设置确认回调：选择军队后请求用户列表
    armyUI->setOnConfirmed([this]() {
        CCLOG("✅ 军队准备完成，开始搜索对手...");
        _uiController->showHint("正在搜索对手...");

        // 🆕 第二步：请求可攻击的用户列表
        auto& client = SocketClient::getInstance();
        if (client.isConnected())
        {
            client.requestUserList();
        }
        else
        {
            // 本地模式：直接从AccountManager获取用户列表
            showLocalPlayerList();
        }
    });

    // 设置取消回调
    armyUI->setOnCancelled([this]() {
        CCLOG("❌ 取消攻击");
        _uiController->showHint("已取消攻击");
    });

    armyUI->show();
}

    // 收集除当前账号外的所有候选目标
    std::vector<AccountInfo> candidates;
    for (const auto& a : allAccounts)
    {
        if (a.userId != currentAccount->userId)
            candidates.push_back(a);
    }

    if (candidates.empty())
    {
        _uiController->showHint("测试模式：请先创建第二个账号！");
        return;
    }

    // 创建半透明遮罩和选择面板
    auto vs = Director::getInstance()->getVisibleSize();
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setContentSize(vs);
    this->addChild(mask, 10000);

    auto panel = ui::Layout::create();
    panel->setContentSize(Size(520, 360));
    panel->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    panel->setBackGroundColor(Color3B(40, 40, 50));
    panel->setPosition(Vec2(vs.width / 2 - 260, vs.height / 2 - 180));
    mask->addChild(panel);

    auto title = Label::createWithSystemFont("选择掠夺目标", "Microsoft YaHei", 24);
    title->setPosition(Vec2(260, 320));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    auto closeBtn = ui::Button::create();
    closeBtn->setTitleText("X");
    closeBtn->setTitleFontSize(24);
    closeBtn->setPosition(Vec2(500, 320));
    closeBtn->addClickEventListener([mask](Ref*) { mask->removeFromParent(); });
    panel->addChild(closeBtn);
        _uiController->showHint("创建战斗场景失败！");
    }
}
        _uiController->showHint("创建战斗场景失败！");
    }
}

    auto list = ui::ListView::create();
    list->setContentSize(Size(480, 260));
    list->setPosition(Vec2(20, 40));
    list->setScrollBarEnabled(true);
    list->setBackGroundColor(Color3B(30,30,30));
    list->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    panel->addChild(list);

    // 填充候选账号
    for (const auto& acc : candidates)
    {
        auto item = ui::Layout::create();
        item->setContentSize(Size(460, 60));
        item->setBackGroundColor(Color3B(50,50,60));

void DraggableMapScene::onCancelBuilding()
{
    if (_buildingManager)
    {
        _buildingManager->cancelBuilding();
    }
    _uiController->hideConfirmButtons();
    _uiController->showHint("已取消建造，点击地图重新选择位置");
}

// ==================== 建筑回调 ====================

void DraggableMapScene::onBuildingPlaced(BaseBuilding* building)
{
    if (!building)
        return;

    CCLOG("Building placed: %s", building->getDisplayName().c_str());

    // ✅ 检查是否为资源生产建筑，如果是则注册用于收集
    auto resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
    if (resourceBuilding && resourceBuilding->isProducer())
    {
        registerResourceBuilding(resourceBuilding);
    }

    // 检查是否为兵营建筑
    if (building->getBuildingType() == BuildingType::kArmy)
    {
        auto barracks = dynamic_cast<ArmyBuilding*>(building);
        if (barracks)
        {
            barracks->setOnTrainingComplete([this, barracks](Unit* unit) {
                if (!unit)
                    return;

                Vec2 barracksWorldPos = barracks->getParent()->convertToWorldSpace(barracks->getPosition());
                Vec2 spawnPos         = barracksWorldPos;
                spawnPos.x += barracks->getContentSize().width * barracks->getScale() + 20;

                Vec2 spawnLocalPos = _mapController->getMapSprite()->convertToNodeSpace(spawnPos);
                unit->setPosition(spawnLocalPos);

                _mapController->getMapSprite()->addChild(unit, 100);
                unit->PlayAnimation(UnitAction::kIdle, UnitDirection::kRight);

                CCLOG("?? Unit training complete!");
                _uiController->showHint("士兵训练完成！");
            });
        }
    }

    _uiController->hideConfirmButtons();
}
        // 点击直接开始攻击选中玩家
        item->setTouchEnabled(true);
        item->addClickEventListener([this, acc, mask](Ref*) {
            // 关闭面板
            if (mask)
                mask->removeFromParent();
    
void DraggableMapScene::onBuildingClicked(BaseBuilding* building)
{
    if (!building)
        return;

    showUpgradeUI(building);
}

void DraggableMapScene::onBuildingHint(const std::string& hint)
{
    _uiController->showHint(hint);
}
            // 加载目标玩家数据并进入战斗（原有逻辑）
            auto enemyGameData = AccountManager::getInstance().getPlayerGameData(acc.userId);
            if (enemyGameData.buildings.empty())
            {
                _uiController->showHint(StringUtils::format("玩家 %s 还没有建筑！", acc.userId.c_str()));
                return;
            }
{
// ==================== 升级UI ====================

void DraggableMapScene::showUpgradeUI(BaseBuilding* building)
{
    hideUpgradeUI();

    auto upgradeUI = BuildingUpgradeUI::create(building);
    if (upgradeUI)
    {
        upgradeUI->setPositionNearBuilding(building);

        upgradeUI->setUpgradeCallback([this, building](bool success, int newLevel) {
            if (success)
            CCLOG("✅ Attacking player: %s (TH Level=%d, Buildings=%zu)",
                  acc.userId.c_str(), enemyGameData.townHallLevel, enemyGameData.buildings.size());

            auto battleScene = BattleScene::createWithEnemyData(enemyGameData);
            if (battleScene)
        
        upgradeUI->setUpgradeCallback([this, building](bool success, int newLevel) {
            if (success)
        
        upgradeUI->setUpgradeCallback([this, building](bool success, int newLevel) {
            if (success)
            {
                Director::getInstance()->pushScene(TransitionFade::create(0.3f, battleScene));

        upgradeUI->setCloseCallback([this]() { _currentUpgradeUI = nullptr; });

        this->addChild(upgradeUI, 1000);
        upgradeUI->show();
        _currentUpgradeUI = upgradeUI;
    }
}

void DraggableMapScene::hideUpgradeUI()
{
    if (!_currentUpgradeUI)
        return;

    auto upgradeUI = dynamic_cast<BuildingUpgradeUI*>(_currentUpgradeUI);
    if (upgradeUI)
    {
        upgradeUI->hide();
    }
    else
    {
        _currentUpgradeUI->removeFromParent();
    }
    _currentUpgradeUI = nullptr;
}

void DraggableMapScene::closeUpgradeUI()
{
    hideUpgradeUI();
}

void DraggableMapScene::cleanupUpgradeUI()
{
    if (_currentUpgradeUI)
    {
        if (_currentUpgradeUI->getParent() == this)
        {
            _currentUpgradeUI->removeFromParent();
        }
        _currentUpgradeUI = nullptr;
    }
}

// ==================== ✅ 资源建筑注册 ====================

void DraggableMapScene::registerResourceBuilding(ResourceBuilding* building)
{
    if (_collectionMgr && building)
    {
        _collectionMgr->registerBuilding(building);
        CCLOG("✅ 注册资源建筑收集：%s", building->getDisplayName().c_str());
    }
}

// ==================== 网络 ====================

void DraggableMapScene::connectToServer()
{
    auto& client = SocketClient::getInstance();

    if (!client.isConnected())
    {
        bool connected = client.connect("127.0.0.1", 8888);

        if (connected)
        {
            auto account = AccountManager::getInstance().getCurrentAccount();
            if (account)
            {
                client.login(account->userId, account->username, 1000);
            }
        }
    }
}

void DraggableMapScene::setupNetworkCallbacks()
{
    auto& client = SocketClient::getInstance();

    client.setOnLoginResult([](bool success, const std::string& msg) {
        if (success)
        {
            CCLOG("Login successful!");
        }
        else
        {
            CCLOG("Login failed: %s", msg.c_str());
        }
    });

    // 🆕 设置用户列表回调
    client.setOnUserListReceived([this](const std::string& userListData) {
        CCLOG("✅ 收到用户列表数据: %s", userListData.c_str());
        showPlayerListFromServerData(userListData);
    });

    // 🆕 设置攻击结果回调
    client.setOnAttackResult([this](const AttackResult& result) {
        CCLOG("🛡️ 收到被攻击通知：攻击者=%s，失去金币=%d，圣水=%d", result.attackerId.c_str(), result.goldLooted,
              result.elixirLooted);

        // 添加到防守日志
        DefenseLog log;
        log.attackerId   = result.attackerId;
        log.attackerName = result.attackerId; // TODO: 从服务器获取真实名称
        log.starsLost    = result.starsEarned;
        log.goldLost     = result.goldLooted;
        log.elixirLost   = result.elixirLooted;
        log.trophyChange = -result.trophyChange;
        log.timestamp    = getCurrentTimestamp();
        log.isViewed     = false;

        DefenseLogSystem::getInstance().addDefenseLog(log);

        // 显示提示
        _uiController->showHint(StringUtils::format("你被 %s 攻击了！失去金币 %d，圣水 %d", log.attackerName.c_str(),
                                                    log.goldLost, log.elixirLost));
    });

    client.setOnDisconnected([]() { CCLOG("Disconnected from server!"); });
}

// ==================== ShopLayer 接口 ====================

int DraggableMapScene::getTownHallLevel() const
{
    if (!_buildingManager)
        return 1;

    const auto& buildings = _buildingManager->getBuildings();
    for (auto* b : buildings)
    {
        if (b->getBuildingType() == BuildingType::kTownHall)
        {
            return b->getLevel();
        }
    }

    return 1;
}

int DraggableMapScene::getBuildingCount(const std::string& name) const
{
    if (!_buildingManager)
        return 0;

    int         count     = 0;
    const auto& buildings = _buildingManager->getBuildings();

    for (auto* b : buildings)
    {
        if (name == "Town Hall" || name == "大本营")
        {
            if (b->getBuildingType() == BuildingType::kTownHall)
            {
                count++;
            }
            continue;
        }

        std::string displayName = b->getDisplayName();
        if (displayName.find(name) != std::string::npos)
        {
            count++;
        }
    }

    return count;
}

void DraggableMapScene::startPlacingBuilding(const BuildingData& data)
{
    if (_buildingManager)
    {
        _buildingManager->startPlacing(data);
    }
}

// ==================== 生命周期 ====================

void DraggableMapScene::update(float dt)
{
    SocketClient::getInstance().processCallbacks();
}

DraggableMapScene::~DraggableMapScene()
{
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->removeFromParent();
        _currentUpgradeUI = nullptr;
    }

    if (_buildingManager && !_isAttackMode)
    {
        _buildingManager->saveCurrentState();
        CCLOG("? Game state auto-saved on scene destruction");
    }
}

// ==================== ✅ 场景恢复处理 ====================

void DraggableMapScene::onSceneResume()
{
    CCLOG("🔄 Scene resumed, cleaning up ResourceCollectionManager...");
    
    // ✅ 关键：从战斗场景返回时，清理 ResourceCollectionManager 中的野指针
    // 因为战斗场景的建筑已经被删除了
    ResourceCollectionManager::getInstance()->clearRegisteredBuildings();
    
    // ✅ 重新注册当前场景的资源建筑
    if (_buildingManager)
    {
        const auto& buildings = _buildingManager->getBuildings();
        for (auto* building : buildings)
        {
            auto resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
            if (resourceBuilding && resourceBuilding->isProducer())
            {
                ResourceCollectionManager::getInstance()->registerBuilding(resourceBuilding);
            }
        }
    }
    
    CCLOG("✅ ResourceCollectionManager cleaned and re-registered current buildings");
}

// ==================== 多人游戏（未使用，保留接口） ====================

bool DraggableMapScene::switchToAttackMode(const std::string& targetUserId)
{
    // 预留接口
    return false;
}

void DraggableMapScene::returnToOwnBase()
{
    // 预留接口
}

void DraggableMapScene::onAccountSwitched()
{
    CCLOG("✅ Account switch initiated...");

    // 1. 保存当前账号的建筑状态和资源
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved current account state");
    }

    // 2. 获取目标账号ID
    std::string targetUserId = UserDefault::getInstance()->getStringForKey("switching_to_account", "");
    if (targetUserId.empty())
    {
        CCLOG("❌ No target account specified");
        return;
    }

    // 3. 切换账号
    auto& accMgr = AccountManager::getInstance();
    if (!accMgr.switchAccount(targetUserId))
    {
        CCLOG("❌ Failed to switch account");
        return;
    }

    CCLOG("✅ Account switched successfully, reloading scene...");

    // 4. 清除临时数据
    UserDefault::getInstance()->setStringForKey("switching_to_account", "");
    UserDefault::getInstance()->flush();

    // 5. 重新创建整个场景以确保彻底清理
    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.3f, newScene));
}

void DraggableMapScene::onLogout()
{
    // 保存当前状态
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
    }

    // 退出游戏或返回主菜单（暂时直接退出）
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

        list->pushBackCustomItem(item);
    }
        exit(0);
    // 重新创建场景以应用新地图
    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, newScene));
    // 重新创建场景以应用新地图
    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, newScene));
}

// ==================== 🆕 本地玩家列表显示 ====================

void DraggableMapScene::showLocalPlayerList()
{
    auto&       accMgr         = AccountManager::getInstance();
    const auto& allAccounts    = accMgr.listAccounts();
    const auto* currentAccount = accMgr.getCurrentAccount();

    if (!currentAccount)
    {
        _uiController->showHint("错误：未登录账号！");
        return;
    }

    // 构建玩家列表（排除当前账号）
    std::vector<PlayerInfo> players;
    for (const auto& account : allAccounts)
    {
        if (account.userId != currentAccount->userId)
        {
            auto gameData = accMgr.getPlayerGameData(account.userId);

            PlayerInfo player;
            player.userId        = account.userId;
            player.username      = account.username;
            player.townHallLevel = gameData.townHallLevel;
            player.trophies      = 1000; // 默认奖杯数
            player.gold          = gameData.gold;
            player.elixir        = gameData.elixir;

            players.push_back(player);
        }
    }

    if (players.empty())
    {
        _uiController->showHint("暂无可攻击的玩家！请先创建其他账号。");
        return;
    }

    // 显示玩家列表UI
    auto playerListLayer = PlayerListLayer::create(players);
    if (playerListLayer)
    {
        this->addChild(playerListLayer, 300);

        // 设置玩家选择回调
        playerListLayer->setOnPlayerSelected([this](const std::string& targetUserId) { startAttack(targetUserId); });

        playerListLayer->show();
    }
}

// ==================== 🆕 服务器玩家列表显示 ====================

void DraggableMapScene::showPlayerListFromServerData(const std::string& serverData)
{
    // 解析服务器返回的用户列表
    // 格式: "userId1,username1,thLevel1,gold1,elixir1|userId2,username2,thLevel2,gold2,elixir2|..."

    std::vector<PlayerInfo> players;
    std::istringstream      iss(serverData);
    std::string             playerStr;

    while (std::getline(iss, playerStr, '|'))
    {
        std::istringstream piss(playerStr);
        std::string        token;

        PlayerInfo player;
        std::getline(piss, player.userId, ',');
        std::getline(piss, player.username, ',');
        std::getline(piss, token, ',');
        if (!token.empty())
            player.townHallLevel = std::stoi(token);
        std::getline(piss, token, ',');
        if (!token.empty())
            player.gold = std::stoi(token);
        std::getline(piss, token, ',');
        if (!token.empty())
            player.elixir = std::stoi(token);

        players.push_back(player);
    }

    if (players.empty())
    {
        _uiController->showHint("暂无可攻击的玩家！");
        return;
    }

    // 显示玩家列表UI
    auto playerListLayer = PlayerListLayer::create(players);
    if (playerListLayer)
    {
        this->addChild(playerListLayer, 300);

        // 设置玩家选择回调
        playerListLayer->setOnPlayerSelected([this](const std::string& targetUserId) { startAttack(targetUserId); });

        playerListLayer->show();
    }
}

// ==================== 🆕 开始攻击 ====================

void DraggableMapScene::startAttack(const std::string& targetUserId)
{
    CCLOG("⚔️ 开始攻击玩家: %s", targetUserId.c_str());
    _uiController->showHint(StringUtils::format("正在加载 %s 的基地...", targetUserId.c_str()));

    auto& accMgr        = AccountManager::getInstance();
    auto  enemyGameData = accMgr.getPlayerGameData(targetUserId);

    if (enemyGameData.buildings.empty())
    {
        _uiController->showHint(StringUtils::format("玩家 %s 还没有建筑！", targetUserId.c_str()));
        return;
    }

    // 进入战斗场景
    CCLOG("✅ 加载成功，进入战斗场景 (TH Level=%d, Buildings=%zu)", enemyGameData.townHallLevel,
          enemyGameData.buildings.size());

    auto battleScene = BattleScene::createWithEnemyData(enemyGameData);
    if (battleScene)
    {
        // ✅ 使用 pushScene 而不是 replaceScene，这样返回时可以保留当前场景的状态
        Director::getInstance()->pushScene(TransitionFade::create(0.3f, battleScene));
    }
    else
    {
        _uiController->showHint("创建战斗场景失败！");
    }
}

// ==================== 🆕 获取当前时间戳 ====================

std::string DraggableMapScene::getCurrentTimestamp()
{
    time_t now = time(nullptr);
    char   buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}
