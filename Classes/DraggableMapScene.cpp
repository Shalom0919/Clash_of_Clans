/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        刘相成
 * Update Date:   2025/12/2
 * License:       MIT License
 ****************************************************************/
#include "DraggableMapScene.h"
#include "cocos2d.h"
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

    // 初始化成员变量
    _visibleSize = Director::getInstance()->getVisibleSize();
    _currentScale = 1.3f;
    _minScale = 1.0f;
    _maxScale = 2.5f;

    // 初始化地图列表
    _mapNames = { "map/Map1.png", "map/Map2.png", "map/Map3.png" };
    _currentMapName = "map/Map1.png"; // 默认地图
    _mapSprite = nullptr;
    _gridMap = nullptr;
    _lastTouchPos = Vec2::ZERO;
    _dragStartPos = Vec2::ZERO;

    _isBuildingMode = false;
    _isDraggingBuilding = false;
    _ghostSprite = nullptr;
    _selectedBuilding = BuildingData(); // 使用默认构造函数

    _buildButton = nullptr;
    _mapButton = nullptr;
    _buildingListUI = nullptr;
    _mapList = nullptr;
    _isBuildingListVisible = false;
    _isMapListVisible = false;

    _heroManager = nullptr;


    // 初始化建筑数据
    initBuildingData();

    // 初始化每张地图的校准配置：scale, startPixel, tileSize
    _mapConfigs.clear();
    _mapConfigs["map/Map1.png"] = { 1.3f, Vec2(1406.0f, 2107.2f), 55.6f };
    _mapConfigs["map/Map2.png"] = { 1.3f, Vec2(1403.0f, 2090.2f), 55.6f };
    _mapConfigs["map/Map3.png"] = { 1.3f, Vec2(1401.0f, 2077.2f), 55.6f };

    // 创建英雄管理器
    _heroManager = HeroManager::create();
    this->addChild(_heroManager);

    setupMap();
    setupUI();
    setupTouchListener();

    // 添加键盘监听器
    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE) {
            if (_isBuildingMode) {
                this->cancelPlacing();
            }
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    setupMouseListener();

    return true;
}
void DraggableMapScene::showBuildingHint(const std::string& hint)
{
    // 移除旧的提示
    auto oldHint = this->getChildByName("buildingHint");
    if (oldHint) {
        oldHint->removeFromParent();
    }

    // 创建新的提示
    auto hintLabel = Label::createWithSystemFont(hint, "Arial", 18);
    hintLabel->setPosition(Vec2(_visibleSize.width / 2, 100));
    hintLabel->setTextColor(Color4B::YELLOW);
    hintLabel->setName("buildingHint");
    this->addChild(hintLabel, 30);
}

cocos2d::Vec2 DraggableMapScene::calculateBuildingPosition(const cocos2d::Vec2& gridPos)
{
    if (!_gridMap) {
        return Vec2::ZERO;
    }

    // 计算建筑中心位置
    Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
    Vec2 posEnd = _gridMap->getPositionFromGrid(gridPos +
        Vec2(_selectedBuilding.gridSize.width - 1, _selectedBuilding.gridSize.height - 1));
    Vec2 centerPos = (posStart + posEnd) / 2.0f;

    return centerPos;
}
void DraggableMapScene::initBuildingData()
{
    _buildingList.clear();
    // 根据网格尺寸设置不同的缩放比例
    // 3x3建筑用0.8，2x2用1.0，4x4用0.6
    _buildingList.push_back(BuildingData("箭塔", "Tower.png", Size(3, 3), 0.8f, 1000, 60));
    _buildingList.push_back(BuildingData("炮塔", "Cannon.png", Size(2, 2), 1.0f, 500, 30));
    _buildingList.push_back(BuildingData("兵营", "Barracks.png", Size(4, 4), 0.6f, 1500, 120));
    _buildingList.push_back(BuildingData("金矿", "GoldMine.png", Size(3, 3), 0.8f, 800, 45));
    _buildingList.push_back(BuildingData("圣水收集器", "ElixirCollector.png", Size(3, 3), 0.8f, 750, 40));
}
// 添加缺失的 setupMap 方法
void DraggableMapScene::setupMap()
{
    _mapSprite = Sprite::create(_currentMapName);
    if (_mapSprite) {
        auto mapSize = _mapSprite->getContentSize();
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        this->addChild(_mapSprite, 0);

        // --------------------------------------------------------
        // 传入小格子的尺寸！
        // 假设原本 100 是大格子，现在我们把大格子切成 3x3
        // --------------------------------------------------------
        _gridMap = GridMap::create(mapSize, 55.6f);
        _mapSprite->addChild(_gridMap, 999);

        // 创建 GridMap 时使用该地图的配置（如存在）
        float tile = 55.6f;
        Vec2 startPixel = Vec2::ZERO;
        float mapScale = _currentScale;
        auto it = _mapConfigs.find(_currentMapName);
        if (it != _mapConfigs.end()) {
            tile = it->second.tileSize;
            startPixel = it->second.startPixel;
            mapScale = it->second.scale;
        }

        _currentScale = mapScale;
        _mapSprite->setScale(_currentScale);

        _gridMap = GridMap::create(mapSize, tile);
        _mapSprite->addChild(_gridMap, 999);

        if (_gridMap && startPixel != Vec2::ZERO) {
            _gridMap->setStartPixel(startPixel);
            _gridStartDefault = startPixel;
        } else if (_gridMap) {
            _gridStartDefault = _gridMap->getStartPixel();
        }

        updateBoundary();
        createSampleMapElements();
    }
    else {
        CCLOG("Error: Failed to load map image %s", _currentMapName.c_str());

        auto errorLabel = Label::createWithSystemFont(
            "Failed to load " + _currentMapName, "Arial", 32);
        errorLabel->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        errorLabel->setTextColor(Color4B::RED);
        this->addChild(errorLabel);
    }

    // 添加背景色
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);
}

void DraggableMapScene::setupUI()
{
    // Build按钮 - 放在左上角
    _buildButton = Button::create();
    _buildButton->setTitleText("Build");
    _buildButton->setTitleFontSize(24);
    _buildButton->setContentSize(Size(100, 50));
    _buildButton->setPosition(Vec2(80, _visibleSize.height - 50));
    _buildButton->addClickEventListener([this](Ref* sender) {
    buildBtn->addClickEventListener([this](Ref* sender) {
        if (_isBuildingMode) {
            this->cancelPlacing();
        }
        else {
            this->toggleBuildingSelection();
        }
        });
    this->addChild(_buildButton, 10);
    
    
    // 地图切换按钮
    _mapButton = Button::create();
    _mapButton->setTitleText("Map");
    _mapButton->setTitleFontSize(24);
    _mapButton->setContentSize(Size(120, 60));
    _mapButton->setPosition(Vec2(_visibleSize.width - 80, _visibleSize.height - 50));
    _mapButton->addClickEventListener(CC_CALLBACK_1(DraggableMapScene::onMapButtonClicked, this));
    this->addChild(_mapButton, 10);

    // 设置英雄UI
    _heroManager->setupHeroUI(this, _visibleSize);

    // 创建建筑选择栏
    createBuildingSelection();

    // 创建地图列表
    createMapList();

    // 操作提示
    auto tipLabel = Label::createWithSystemFont(
        "Drag: Move Map  Scroll: Zoom  Buttons: Switch Map/Hero/Build\nClick Hero to Select, Click Ground to Move",
        "Arial", 14);
    tipLabel->setPosition(Vec2(_visibleSize.width / 2, 40));
    tipLabel->setTextColor(Color4B::YELLOW);
    tipLabel->setAlignment(TextHAlignment::CENTER);
    this->addChild(tipLabel, 10);

    // 当前地图名称显示
    auto mapNameLabel = Label::createWithSystemFont("Current: " + _currentMapName, "Arial", 18);
    mapNameLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height - 30));
    mapNameLabel->setTextColor(Color4B::GREEN);
    mapNameLabel->setName("mapNameLabel");
    this->addChild(mapNameLabel, 10);

    // --- 添加微调 UI: 四个箭头按钮 + 重置 ---
    Vec2 uiBase = Vec2(500, 400);
    float btnSize = 40.0f;

    auto makeArrowBtn = [this, btnSize](const std::string& title, const Vec2& pos, const std::function<void()>& cb) {
        auto btn = ui::Button::create();
        btn->setTitleText(title);
        btn->setTitleFontSize(18);
        btn->setContentSize(Size(btnSize, btnSize));
        btn->setScale9Enabled(true);
        btn->setPosition(pos);
        btn->addClickEventListener([cb](Ref* sender) { cb(); });
        this->addChild(btn, 30);
        return btn;
    };

    // 左
    makeArrowBtn("←", uiBase + Vec2(-50, 0), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(-1, 0);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    // 右
    makeArrowBtn("→", uiBase + Vec2(50, 0), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(1, 0);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    // 上
    makeArrowBtn("↑", uiBase + Vec2(0, 50), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(0, 1);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    // 下
    makeArrowBtn("↓", uiBase + Vec2(0, -50), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(0, -1);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    // 重置按钮
    makeArrowBtn("Reset", uiBase + Vec2(0, -110), [this]() {
        if (!_gridMap) return;
        _gridMap->setStartPixel(_gridStartDefault);
        _gridMap->showWholeGrid(true);
        Vec2 p = _gridMap->getStartPixel();
        CCLOG("Grid reset to default: %.2f, %.2f", p.x, p.y);
    });
}

void DraggableMapScene::toggleBuildingSelection()
{
    _isBuildingListVisible = !_isBuildingListVisible;
    _buildingListUI->setVisible(_isBuildingListVisible);

    // 如果打开建筑列表，关闭其他列表
    if (_isBuildingListVisible) {
        if (_isMapListVisible) {
            toggleMapList();
        }
        if (_heroManager->isHeroListVisible()) {
            _heroManager->hideHeroList();
        }
    }
}
void DraggableMapScene::createBuildingSelection()
{
    _buildingListUI = ListView::create();
    _buildingListUI->setContentSize(Size(300, 200));
    _buildingListUI->setPosition(Vec2(160, _visibleSize.height - 250)); // 左上角下方
    _buildingListUI->setBackGroundColor(Color3B(60, 60, 80));
    _buildingListUI->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    _buildingListUI->setOpacity(220);
    _buildingListUI->setVisible(false);
    _buildingListUI->setScrollBarEnabled(true);
    _buildingListUI->setBounceEnabled(true);

    for (const auto& building : _buildingList) {
        auto item = Layout::create();
        item->setContentSize(Size(280, 60));
        item->setTouchEnabled(true);

        // 建筑图标
        auto buildingSprite = Sprite::create(building.imageFile);
        if (buildingSprite) {
            buildingSprite->setScale(0.3f);
            buildingSprite->setPosition(Vec2(40, 30));
            buildingSprite->setName("sprite");
            item->addChild(buildingSprite);
        }

        // 建筑信息
        auto nameLabel = Label::createWithSystemFont(building.name, "Arial", 16);
        nameLabel->setPosition(Vec2(120, 40));
        nameLabel->setTextColor(Color4B::YELLOW);
        nameLabel->setName("name");
        item->addChild(nameLabel);

        // 建筑尺寸标签
        std::string sizeText = StringUtils::format("%dx%d",
            (int)building.gridSize.width, (int)building.gridSize.height);
        auto sizeLabel = Label::createWithSystemFont(sizeText, "Arial", 14);
        sizeLabel->setPosition(Vec2(120, 20));
        sizeLabel->setTextColor(Color4B::GREEN);
        item->addChild(sizeLabel);

        // 建筑费用
        std::string costText = StringUtils::format("Cost: %d", (int)building.cost);
        auto costLabel = Label::createWithSystemFont(costText, "Arial", 12);
        costLabel->setPosition(Vec2(220, 40));
        costLabel->setTextColor(Color4B::WHITE);
        item->addChild(costLabel);

        // 添加背景色
        auto itemBg = LayerColor::create(Color4B(40, 40, 60, 255));
        itemBg->setContentSize(Size(280, 60));
        itemBg->setPosition(Vec2::ZERO);
        item->addChild(itemBg, -1);

        // 添加点击事件 - 使用lambda捕获building
        item->addClickEventListener([this, building](Ref* sender) {
            this->onBuildingItemClicked(sender, building);
            });

        _buildingListUI->pushBackCustomItem(item);
    }

    this->addChild(_buildingListUI, 20);
}
void DraggableMapScene::onBuildingItemClicked(cocos2d::Ref* sender, const BuildingData& building)
{
    CCLOG("Selected building: %s, Size: %.0fx%.0f",
        building.name.c_str(), building.gridSize.width, building.gridSize.height);

    // 开始放置该建筑
    startPlacingBuilding(building);

    // 隐藏建筑列表
    toggleBuildingSelection();
}
void DraggableMapScene::createMapList()
{
    _mapList = ListView::create();
    _mapList->setContentSize(Size(150, 200));
    _mapList->setPosition(Vec2(_visibleSize.width - 160, _visibleSize.height - 240));
    _mapList->setBackGroundColor(Color3B(80, 80, 80));
    _mapList->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    _mapList->setOpacity(200);
    _mapList->setVisible(false);
    _mapList->setScrollBarEnabled(true);

    for (const auto& mapName : _mapNames) {
        auto item = Layout::create();
        item->setContentSize(Size(140, 40));
        item->setTouchEnabled(true);

        auto label = Label::createWithSystemFont(mapName, "Arial", 16);
        label->setPosition(Vec2(70, 20));
        label->setTextColor(Color4B::WHITE);
        label->setName("label");
        item->addChild(label);

        // 添加点击事件
        item->addClickEventListener([this, mapName](Ref* sender) {
            this->onMapItemClicked(sender);
            });

        _mapList->pushBackCustomItem(item);
    }

    this->addChild(_mapList, 20);
}

void DraggableMapScene::onMapButtonClicked(cocos2d::Ref* sender)
{
    toggleMapList();
}

void DraggableMapScene::toggleMapList()
{
    _isMapListVisible = !_isMapListVisible;
    _mapList->setVisible(_isMapListVisible);

    // 如果打开地图列表，关闭英雄列表
    if (_isMapListVisible && _heroManager->isHeroListVisible()) {
        _heroManager->hideHeroList();
    }
}

void DraggableMapScene::onMapItemClicked(cocos2d::Ref* sender)
{
    auto item = static_cast<Layout*>(sender);
    auto label = static_cast<Label*>(item->getChildByName("label"));
    std::string selectedMapName = label->getString();

    CCLOG("Selected map: %s", selectedMapName.c_str());

    // 切换地图
    switchMap(selectedMapName);

    // 隐藏地图列表
    toggleMapList();
}

void DraggableMapScene::switchMap(const std::string& mapName)
{
    if (mapName == _currentMapName) return;

    /*
    如果在建造模式点击了 “切换地图”：
    switchMap 被调用。
    this->removeChild(_mapSprite) 被执行。
    _mapSprite 被移除，它的所有子节点（包括 _ghostSprite）都会被自动 Cleanup 并释放内存。
    此时 _ghostSprite 指向的对象已经死了（内存变成了 0xDDDDDDDD）。
    但是！ 你并没有把 _isBuildingMode 设为 false，也没有把 _ghostSprite 指针置为 nullptr。
    当你再次移动鼠标 (onTouchMoved) 或者点击屏幕 (onTouchEnded) 时，代码检查 if (_ghostSprite)，
    发现指针不为空（还存着旧地址），于是尝试操作它 -> 崩溃。
    */
    if (_isBuildingMode) {
        cancelPlacing();
    }

    // ... (保存状态、移除旧地图代码保持不变) ...
    saveMapElementsState();

    if (_mapSprite) {
        this->removeChild(_mapSprite);
        _mapSprite = nullptr;
        _gridMap = nullptr; // 指针置空
    }

    // 3. 创建新地图
    _currentMapName = mapName;
    _mapSprite = Sprite::create(_currentMapName);

    if (_mapSprite) {
        // 设置位置和缩放
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        _mapSprite->setScale(_currentScale);
        this->addChild(_mapSprite, 0);

        // --------------------------------------------------------
        // 【新增代码】给新地图添加网格
        // --------------------------------------------------------
        auto mapSize = _mapSprite->getContentSize();
        _gridMap = GridMap::create(mapSize, 55.6f);
        _mapSprite->addChild(_gridMap, 999);

        // 使用每张地图的配置（如果存在）
        float tile = 55.6f;
        Vec2 startPixel = Vec2::ZERO;
        float mapScale = _currentScale;
        auto it = _mapConfigs.find(_currentMapName);
        if (it != _mapConfigs.end()) {
            tile = it->second.tileSize;
            startPixel = it->second.startPixel;
            mapScale = it->second.scale;
        }

        _currentScale = mapScale;
        _mapSprite->setScale(_currentScale);

        _gridMap = GridMap::create(mapSize, tile);
        _mapSprite->addChild(_gridMap, 999);

        if (_gridMap && startPixel != Vec2::ZERO) {
            _gridMap->setStartPixel(startPixel);
            _gridStartDefault = startPixel;
        } else if (_gridMap) {
            _gridStartDefault = _gridMap->getStartPixel();
        }
        // --------------------------------------------------------

        // 更新边界
        updateBoundary();

        // 恢复地图元素
        restoreMapElementsState();

        // 5. 通知英雄管理器地图已切换
        _heroManager->onMapSwitched(_mapSprite);
        _heroManager->updateHeroesScale(_currentScale);  // 立即更新英雄缩放

        // 6. 更新UI显示
        auto mapNameLabel = static_cast<Label*>(this->getChildByName("mapNameLabel"));
        if (mapNameLabel) {
            mapNameLabel->setString("Current: " + _currentMapName);
        }

        CCLOG("Map switched successfully to %s", mapName.c_str());
    }
    else {
        CCLOG("Error: Failed to load new map %s", mapName.c_str());

        // 恢复旧地图名称
        _currentMapName = "Map7.png"; // 回退到默认地图
        _mapSprite = Sprite::create(_currentMapName);
        if (_mapSprite) {
            _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
            _mapSprite->setScale(_currentScale);
            this->addChild(_mapSprite, 0);
            updateBoundary();
            restoreMapElementsState();
        }
    }
}

void DraggableMapScene::saveMapElementsState()
{
    // 保存元素的本地坐标
    for (auto& element : _mapElements) {
        if (element.node) {
            element.localPosition = element.node->getPosition();
            element.node->retain(); // 保持引用，防止被自动释放
        }
    }
}

void DraggableMapScene::restoreMapElementsState()
{
    // 恢复地图元素到新地图上
    for (auto& element : _mapElements) {
        if (element.node && element.node->getParent() == nullptr) {
            _mapSprite->addChild(element.node, 1); // 添加到新地图上
            element.node->setPosition(element.localPosition);
        }
        element.node->release(); // 释放之前保留的引用
    }
}

void DraggableMapScene::createSampleMapElements()
{
    // 清除旧元素
    _mapElements.clear();

    if (!_mapSprite) return;

    // 创建一些示例元素（标记点等）
    auto createMarker = [this](const Vec2& worldPosition, const Color4B& color, const std::string& text) {
        // 将世界坐标转换为地图本地坐标
        Vec2 localPos = _mapSprite->convertToNodeSpace(worldPosition);

        // 标记点
        auto marker = DrawNode::create();
        marker->drawDot(Vec2::ZERO, 10, Color4F(color));
        marker->setPosition(localPos);
        _mapSprite->addChild(marker, 1);  // 添加到地图上，而不是场景上

        // 文字标签
        auto label = Label::createWithSystemFont(text, "Arial", 16);
        label->setPosition(localPos + Vec2(0, 20));
        label->setTextColor(Color4B::WHITE);
        _mapSprite->addChild(label, 1);  // 添加到地图上

        // 保存元素信息
        MapElement markerElement = { marker, localPos };
        MapElement labelElement = { label, localPos + Vec2(0, 20) };
        _mapElements.push_back(markerElement);
        _mapElements.push_back(labelElement);
        };

    // 在世界坐标中创建几个示例标记
    createMarker(Vec2(_visibleSize.width * 0.3f, _visibleSize.height * 0.7f), Color4B::RED, "Point A");
    createMarker(Vec2(_visibleSize.width * 0.7f, _visibleSize.height * 0.5f), Color4B::GREEN, "Point B");
    createMarker(Vec2(_visibleSize.width * 0.5f, _visibleSize.height * 0.3f), Color4B::BLUE, "Point C");

    CCLOG("Created %zd map elements", _mapElements.size());
}

void DraggableMapScene::updateMapElementsPosition()
{
    if (!_mapSprite) return;

    for (auto& element : _mapElements) {
        if (element.node && element.node->getParent() == _mapSprite) {
            // 保持相对于地图的本地位置不变
            element.node->setPosition(element.localPosition);
        }
    }
}

void DraggableMapScene::setupTouchListener()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

    // 使用传统方法绑定
    touchListener->onTouchBegan = CC_CALLBACK_2(DraggableMapScene::onTouchBegan, this);
    touchListener->onTouchMoved = CC_CALLBACK_2(DraggableMapScene::onTouchMoved, this);
    touchListener->onTouchEnded = CC_CALLBACK_2(DraggableMapScene::onTouchEnded, this);
    touchListener->onTouchCancelled = CC_CALLBACK_2(DraggableMapScene::onTouchCancelled, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

void DraggableMapScene::setupMouseListener()
{
    auto mouseListener = EventListenerMouse::create();

    mouseListener->onMouseScroll = [this](Event* event) {
        if (!_mapSprite) return;

        EventMouse* mouseEvent = dynamic_cast<EventMouse*>(event);
        if (mouseEvent) {
            float scrollY = mouseEvent->getScrollY();  // 获取滚轮增量

        // 计算缩放因子（滚轮向下为正，向上为负）
        float zoomFactor = 1.0f;
        if (scrollY < 0) {
            zoomFactor = 1.1f;
        }
        else if (scrollY > 0) {
            zoomFactor = 0.9f;
        }
        else {
            return;  // 没有滚动
        }

            // 获取鼠标当前位置作为缩放中心点
            Vec2 mousePos = Vec2(mouseEvent->getCursorX(), mouseEvent->getCursorY());

            // 执行缩放
            zoomMap(zoomFactor, mousePos);

            CCLOG("Mouse scroll: %.1f, Scale: %.2f", scrollY, _currentScale);
        }
        };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void DraggableMapScene::zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint)
{
    if (!_mapSprite) return;

    // 计算新缩放比例
    float newScale = _currentScale * scaleFactor;

    // 限制缩放范围
    newScale = MAX(_minScale, MIN(_maxScale, newScale));

    if (newScale == _currentScale) {
        return;  // 缩放比例没有变化
    }

    // 保存缩放前的状态
    Vec2 oldPosition = _mapSprite->getPosition();
    Vec2 oldAnchor = _mapSprite->getAnchorPoint();

    // 如果指定了中心点，计算基于该点的缩放
    if (pivotPoint != Vec2::ZERO) {
        // 将中心点转换到地图的本地坐标系
        Vec2 worldPos = pivotPoint;
        Vec2 localPos = _mapSprite->convertToNodeSpace(worldPos);

        // 计算缩放前后的位置变化
        Vec2 offsetBefore = localPos * _currentScale;
        Vec2 offsetAfter = localPos * newScale;
        Vec2 positionDelta = offsetAfter - offsetBefore;

        // 应用缩放和位置调整
        _mapSprite->setScale(newScale);
        _mapSprite->setPosition(oldPosition - positionDelta);
    }
    else {
        // 没有指定中心点，简单缩放
        _mapSprite->setScale(newScale);
    }

    // 更新当前缩放比例
    _currentScale = newScale;

    // 更新边界（缩放后边界会变化）
    updateBoundary();

    // 更新地图元素位置
    updateMapElementsPosition();

    // 更新英雄缩放
    _heroManager->updateHeroesScale(_currentScale);

    // 确保地图在边界内
    ensureMapInBoundary();
}

void DraggableMapScene::updateBoundary()
{
    if (!_mapSprite) return;

    auto mapSize = _mapSprite->getContentSize() * _currentScale;

    // 计算拖动边界
    float minX = _visibleSize.width - mapSize.width / 2;
    float maxX = mapSize.width / 2;
    float minY = _visibleSize.height - mapSize.height / 2;
    float maxY = mapSize.height / 2;

    // 如果地图比屏幕小，则应该居中不能拖动
    if (mapSize.width <= _visibleSize.width) {
        minX = maxX = _visibleSize.width / 2;
    }
    if (mapSize.height <= _visibleSize.height) {
        minY = maxY = _visibleSize.height / 2;
    }

    _mapBoundary = Rect(minX, minY, maxX - minX, maxY - minY);

    CCLOG("Boundary updated - Scale: %.2f, Boundary: minX=%.1f, maxX=%.1f",
        _currentScale, minX, maxX);
}

bool DraggableMapScene::onTouchBegan(Touch* touch, Event* event)
{
    _lastTouchPos = touch->getLocation();

    // 如果正在建造模式但不是拖动状态，这是第一次点击（开始放置）
    if (_isBuildingMode && !_isDraggingBuilding) {
        // 保存拖动起始位置
        _dragStartPos = _lastTouchPos;

        // 开始拖动
        _isDraggingBuilding = true;

        // 将幻影建筑移动到起始位置
        if (_ghostSprite && _gridMap) {
            Vec2 gridPos = _gridMap->getGridPosition(_dragStartPos);
            Vec2 buildingPos = calculateBuildingPosition(gridPos);
            _ghostSprite->setPosition(buildingPos);
            _ghostSprite->setVisible(true);

            // 更新底座显示
            bool canBuild = _gridMap->checkArea(gridPos, _selectedBuilding.gridSize);
            _gridMap->updateBuildingBase(gridPos, _selectedBuilding.gridSize, canBuild);

            // 显示拖动提示
            showBuildingHint("拖动调整位置，再次点击确认放置");
        }

        return true; // 吞噬触摸
    }

    // 如果正在建造模式并且是拖动状态，这是第二次点击（确认放置）
    if (_isBuildingMode && _isDraggingBuilding) {
        // 计算当前位置的网格坐标
        Vec2 gridPos = _gridMap->getGridPosition(_lastTouchPos);

        // 执行放置
        placeBuilding(gridPos);

        // 重置状态
        _isDraggingBuilding = false;

        return true; // 吞噬触摸
    }

    // 首先检查是否点击了英雄（优先处理英雄点击）
    if (!_isBuildingMode && !_heroManager->getSelectedHeroName().empty()) {
        // 如果有待放置的英雄，优先放置英雄
        _heroManager->handleHeroTouch(_lastTouchPos, _mapSprite, true);
        return true;
    }

    // 英雄点击检查（选择或移动英雄）
    bool heroClicked = false;
    if (!_isBuildingMode) {
        _heroManager->handleHeroTouch(_lastTouchPos, _mapSprite, false);

        // 检查是否有英雄被点击
        for (auto& hero : _heroManager->getPlacedHeroes()) {
            if (hero && hero->containsTouch(_lastTouchPos, _mapSprite)) {
                heroClicked = true;
                break;
            }
        }
    }

    return true;
}


void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    Vec2 currentTouchPos = touch->getLocation();

    // 只有在建造模式并且正在拖动时才更新建筑位置
    if (_isBuildingMode && _isDraggingBuilding && _ghostSprite && _gridMap) {
        // 将世界坐标转换为网格坐标
        Vec2 gridPos = _gridMap->getGridPosition(currentTouchPos);

        // 冲突检测
        bool canBuild = _gridMap->checkArea(gridPos, _selectedBuilding.gridSize);

        // 更新底座显示
        _gridMap->updateBuildingBase(gridPos, _selectedBuilding.gridSize, canBuild);

        // 计算建筑位置并更新幻影
        Vec2 buildingPos = calculateBuildingPosition(gridPos);
        _ghostSprite->setPosition(buildingPos);

        // 根据是否可以建造调整颜色
        if (canBuild) {
            _ghostSprite->setColor(Color3B::WHITE);
        }
        else {
            _ghostSprite->setColor(Color3B(255, 100, 100)); // 淡红色
        }

        return;
    }

    // 普通拖拽地图逻辑
    Vec2 delta = currentTouchPos - _lastTouchPos;
    moveMap(delta);
    _lastTouchPos = currentTouchPos;
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event)
{
    
}

void DraggableMapScene::onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event)
{
    this->onTouchEnded(touch, event);
}

void DraggableMapScene::moveMap(const cocos2d::Vec2& delta)
{
    if (!_mapSprite) return;

    _mapSprite->setPosition(_mapSprite->getPosition() + delta);
    ensureMapInBoundary();
}

void DraggableMapScene::ensureMapInBoundary()
{
    if (!_mapSprite) return;

    cocos2d::Vec2 currentPos = _mapSprite->getPosition();
    cocos2d::Vec2 newPos = currentPos;

    if (currentPos.x < _mapBoundary.getMinX()) {
        newPos.x = _mapBoundary.getMinX();
    }
    else if (currentPos.x > _mapBoundary.getMaxX()) {
        newPos.x = _mapBoundary.getMaxX();
    }

    if (currentPos.y < _mapBoundary.getMinY()) {
        newPos.y = _mapBoundary.getMinY();
    }
    else if (currentPos.y > _mapBoundary.getMaxY()) {
        newPos.y = _mapBoundary.getMaxY();
    }

    if (newPos != currentPos) {
        _mapSprite->setPosition(newPos);
    }
}

void DraggableMapScene::startPlacingBuilding(const BuildingData& building)
{
    if (!_mapSprite || !_gridMap) return;

    _isBuildingMode = true;
    _isDraggingBuilding = false; // 初始状态不是拖动
    _selectedBuilding = building;

    // 1. 开启全屏网格显示
    _gridMap->showWholeGrid(true);

    // 创建幻影建筑，但先隐藏或放在角落
    _ghostSprite = Sprite::create(building.imageFile);
    if (_ghostSprite) {
        _ghostSprite->setOpacity(150); // 半透明
        _ghostSprite->setAnchorPoint(Vec2(0.5f, 0.2f)); // 锚点调整，脚底对齐

        // 使用建筑数据中预设的缩放比例
        _ghostSprite->setScale(building.scaleFactor);

        // 初始放在屏幕外，等待拖动开始
        _ghostSprite->setPosition(Vec2(-1000, -1000));

        // 加到地图上，跟随地图缩放
        _mapSprite->addChild(_ghostSprite, 2000);

        // 显示提示信息
        showBuildingHint("点击地图开始放置建筑");
    }
}

void DraggableMapScene::placeBuilding(Vec2 gridPos)
{
    if (!_ghostSprite || !_isBuildingMode || _selectedBuilding.name.empty() || !_gridMap) return;

    // 冲突检测
    bool canBuild = _gridMap->checkArea(gridPos, _selectedBuilding.gridSize);

    if (!canBuild) {
        CCLOG("Cannot build here! Area occupied or out of bounds.");

        // 红色闪烁效果和音效提示
        auto flashRed = TintTo::create(0.1f, 255, 0, 0);
        auto flashNormal = TintTo::create(0.1f, 255, 255, 255);
        auto shake = MoveBy::create(0.05f, Vec2(5, 0));
        auto shakeBack = MoveBy::create(0.05f, Vec2(-10, 0));
        auto shakeEnd = MoveBy::create(0.05f, Vec2(5, 0));
        auto sequence = Sequence::create(
            Spawn::create(flashRed, shake, nullptr),
            Spawn::create(flashNormal, shakeBack, nullptr),
            shakeEnd,
            nullptr
        );
        _ghostSprite->runAction(sequence);

        // 显示错误提示
        showBuildingHint("无法在此处建造！区域被占用或越界");

        return; // 不结束放置，让用户继续调整位置
    }

    // 标记占用
    _gridMap->markArea(gridPos, _selectedBuilding.gridSize, true);

    // 创建实际建筑
    auto building = Sprite::create(_selectedBuilding.imageFile);
    building->setAnchorPoint(Vec2(0.5f, 0.2f));

    // 使用建筑数据中预设的缩放比例
    building->setScale(_selectedBuilding.scaleFactor);

    // 计算建筑位置
    Vec2 buildingPos = calculateBuildingPosition(gridPos);
    building->setPosition(buildingPos);

    // Z-Order: 基于 Y 轴排序
    building->setLocalZOrder(10000 - buildingPos.y);

    _mapSprite->addChild(building);

    // 建造动画
    building->setScale(0.0f);
    auto scaleAction = EaseBackOut::create(ScaleTo::create(0.4f, _selectedBuilding.scaleFactor));
    auto fadeIn = FadeIn::create(0.3f);
    building->runAction(Spawn::create(scaleAction, fadeIn, nullptr));

    // 显示成功提示
    showBuildingHint(StringUtils::format("%s 建造完成！", _selectedBuilding.name.c_str()));

    CCLOG("Building placed: %s at grid (%.0f, %.0f)",
        _selectedBuilding.name.c_str(), gridPos.x, gridPos.y);

    // 短暂延迟后结束放置模式
    auto delay = DelayTime::create(1.0f);
    auto callback = CallFunc::create([this]() {
        endPlacing();
        });
    this->runAction(Sequence::create(delay, callback, nullptr));
}

void DraggableMapScene::cancelPlacing()
{
    if (_isDraggingBuilding) {
        // 如果正在拖动，先取消拖动回到初始状态
        _isDraggingBuilding = false;

        // 隐藏幻影建筑
        if (_ghostSprite) {
            _ghostSprite->setPosition(Vec2(-1000, -1000));
        }

        // 隐藏底座
        if (_gridMap) {
            _gridMap->hideBuildingBase();
        }

        // 显示初始提示
        showBuildingHint("点击地图开始放置建筑");
    }
    else {
        // 如果还没开始拖动，直接结束放置
        endPlacing();
    }
}

// 统一结束建造，确保所有清理都在同一处执行
void DraggableMapScene::endPlacing()
{
    _isBuildingMode = false;
    _isDraggingBuilding = false; // 重置拖动状态
    _selectedBuilding = BuildingData("", "", Size::ZERO);

    // 移除提示
    auto hint = this->getChildByName("buildingHint");
    if (hint) {
        hint->removeFromParent();
    }

    if (_gridMap) {
        // 关闭网格显示
        _gridMap->showWholeGrid(false);
        _gridMap->hideBuildingBase();
    }

    if (_ghostSprite) {
        _ghostSprite->removeFromParent();
        _ghostSprite = nullptr;
    }
}