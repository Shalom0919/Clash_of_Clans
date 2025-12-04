#include "TownHallSystem.h"

USING_NS_CC;

// ==================== TownHallManager 实现 ====================
TownHallManager* TownHallManager::getInstance() {
    static TownHallManager instance;
    instance.initialize();
    return &instance;
}

void TownHallManager::initialize() {
    if (!_levels.empty()) return;

    _levels = {
        {1,  "BaseCamp/town-hall-1.png",  0,       0,     "初始大本营"},
        {2,  "BaseCamp/town-hall-2.png",  1000,    3600,  "二级大本营"},
        {3,  "BaseCamp/town-hall-3.png",  4000,    14400, "三级大本营"},
        {4,  "BaseCamp/town-hall-4.png",  25000,   43200, "四级大本营"},
        {5,  "BaseCamp/town-hall-5.png",  150000,  86400, "五级大本营"},
        {6,  "BaseCamp/town-hall-6.png",  500000,  172800, "六级大本营"},
        {7,  "BaseCamp/town-hall-7.png",  1200000, 259200, "七级大本营"},
        {8,  "BaseCamp/town-hall-8.png",  3000000, 345600, "八级大本营"},
        {9,  "BaseCamp/town-hall-9.png",  4000000, 432000, "九级大本营"},
        {10, "BaseCamp/town-hall-10.png", 6000000, 518400, "十级大本营"},
        {11, "BaseCamp/town-hall-11.png", 8000000, 604800, "十一级大本营"},
        {12, "BaseCamp/town-hall-12.png", 10000000,691200, "十二级大本营"},
        {13, "BaseCamp/town-hall-13.png", 12000000,777600, "十三级大本营"},
        {14, "BaseCamp/town-hall-14.png", 16000000,864000, "十四级大本营"},
        {15, "BaseCamp/town-hall-15.png", 20000000,950400, "十五级大本营"},
        {16, "BaseCamp/town-hall-16.png", 30000000,1036800, "十六级大本营"},
        {17, "BaseCamp/town-hall-17.png", 0,       0,     "满级大本营"}
    };
}

const TownHallManager::TownHallLevel* TownHallManager::getLevel(int level) const {
    if (level < 1 || level > _levels.size()) return nullptr;
    return &_levels[level - 1];
}

const TownHallManager::TownHallLevel* TownHallManager::getNextLevel(int currentLevel) const {
    if (currentLevel < 1 || currentLevel >= _levels.size()) return nullptr;
    return &_levels[currentLevel];
}

bool TownHallManager::canUpgrade(int currentLevel) const {
    return currentLevel >= 1 && currentLevel < _levels.size();
}

int TownHallManager::getUpgradeCost(int currentLevel) const {
    if (!canUpgrade(currentLevel)) return 0;
    return _levels[currentLevel].upgradeCost;
}

std::string TownHallManager::getNextLevelImage(int currentLevel) const {
    if (!canUpgrade(currentLevel)) return "";
    return _levels[currentLevel].imageFile;
}

// ==================== TownHallBuilding 实现 ====================
TownHallBuilding* TownHallBuilding::create(int level) {
    TownHallBuilding* ret = new (std::nothrow) TownHallBuilding();
    if (ret && ret->init(level)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TownHallBuilding::init(int level) {
    _level = std::max(1, std::min(level, 17));

    auto* levelConfig = TownHallManager::getInstance()->getLevel(_level);
    if (!levelConfig) return false;

    if (!Sprite::initWithFile(levelConfig->imageFile)) {
        return false;
    }

    this->setAnchorPoint(Vec2(0.5f, 0.2f));
    this->setScale(0.6f);
    this->setName(getDisplayName());

    return true;
}

void TownHallBuilding::setLevel(int level) {
    level = std::max(1, std::min(level, 17));
    if (_level == level) return;

    _level = level;
    updateAppearance();
}

bool TownHallBuilding::canUpgrade() const {
    return TownHallManager::getInstance()->canUpgrade(_level);
}

int TownHallBuilding::getUpgradeCost() const {
    return TownHallManager::getInstance()->getUpgradeCost(_level);
}

bool TownHallBuilding::upgrade() {
    if (!canUpgrade()) return false;

    int cost = getUpgradeCost();
    auto rm = ResourceManager::GetInstance();
    if (!rm->HasEnough(ResourceType::kGold, cost)) {
        return false;
    }

    if (!rm->ConsumeResource(ResourceType::kGold, cost)) {
        return false;
    }

    _level++;
    updateAppearance();

    return true;
}

std::string TownHallBuilding::getDisplayName() const {
    return "大本营 Lv." + std::to_string(_level);
}

std::string TownHallBuilding::getUpgradeInfo() const {
    if (!canUpgrade()) {
        return "大本营已满级";
    }

    int cost = getUpgradeCost();
    auto* nextLevel = TownHallManager::getInstance()->getNextLevel(_level);
    if (!nextLevel) return "";

    return "升级到 " + nextLevel->description +
        "\n需要: " + std::to_string(cost) + "金币";
}

void TownHallBuilding::updateAppearance() {
    auto* levelConfig = TownHallManager::getInstance()->getLevel(_level);
    if (!levelConfig) return;

    auto texture = Director::getInstance()->getTextureCache()->addImage(levelConfig->imageFile);
    if (texture) {
        this->setTexture(texture);
        this->setName(getDisplayName());
    }
}

// ==================== TownHallUpgradeUI 实现 ====================
TownHallUpgradeUI* TownHallUpgradeUI::create(TownHallBuilding* building) {
    TownHallUpgradeUI* ret = new (std::nothrow) TownHallUpgradeUI();
    if (ret && ret->init(building)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TownHallUpgradeUI::init(TownHallBuilding* building)
{
    if (!Node::init() || !building) return false;

    _building = building;

    setupUI();

    // 修改触摸监听器 - 修复触摸吞噬问题
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);  // 保持true，但只在UI范围内处理

    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (!this->isVisible()) return false;

        // 只处理点击在UI范围内的触摸
        Vec2 touchInNode = this->convertTouchToNodeSpace(touch);
        Rect bbox = this->getBoundingBox();
        bbox.origin = Vec2::ZERO;

        if (bbox.containsPoint(touchInNode)) {
            CCLOG("TownHallUpgradeUI touched inside");
            return true;  // 吞噬UI内的触摸
        }

        return false;  // UI外的触摸不处理
        };

    // 添加触摸结束事件，处理点击外部关闭
    touchListener->onTouchEnded = [this](Touch* touch, Event* event) {
        Vec2 touchInNode = this->convertTouchToNodeSpace(touch);
        Rect bbox = this->getBoundingBox();
        bbox.origin = Vec2::ZERO;

        if (!bbox.containsPoint(touchInNode)) {
            CCLOG("TownHallUpgradeUI touched outside, hiding");
            this->hide();
        }
        };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    this->setVisible(false);

    return true;
}

void TownHallUpgradeUI::setupUI() {
    // 背景
    auto layer = LayerColor::create(Color4B(50, 50, 70, 220), 250, 180);
    layer->setIgnoreAnchorPointForPosition(false);
    layer->setAnchorPoint(Vec2(0.5f, 0.5f));
    layer->setPosition(Vec2::ZERO);
    this->addChild(layer);

    // 标题
    auto title = Label::createWithSystemFont("大本营升级", "Arial", 20);
    title->setPosition(Vec2(0, 70));
    title->setTextColor(Color4B::YELLOW);
    this->addChild(title);

    // 信息标签
    _infoLabel = Label::createWithSystemFont("", "Arial", 16);
    _infoLabel->setPosition(Vec2(0, 20));
    _infoLabel->setTextColor(Color4B::WHITE);
    _infoLabel->setDimensions(220, 60);
    _infoLabel->setHorizontalAlignment(TextHAlignment::CENTER);
    _infoLabel->setVerticalAlignment(TextVAlignment::CENTER);
    this->addChild(_infoLabel);

    // 升级按钮
    _upgradeButton = ui::Button::create();
    _upgradeButton->setTitleText("升级");
    _upgradeButton->setTitleFontSize(18);
    _upgradeButton->setContentSize(Size(100, 40));
    _upgradeButton->setPosition(Vec2(0, -50));
    _upgradeButton->addClickEventListener(CC_CALLBACK_1(TownHallUpgradeUI::onUpgradeClicked, this));
    _upgradeButton->setColor(Color3B(100, 200, 100));
    this->addChild(_upgradeButton);

    // 关闭按钮
    auto closeButton = ui::Button::create();
    closeButton->setTitleText("×");
    closeButton->setTitleFontSize(24);
    closeButton->setContentSize(Size(30, 30));
    closeButton->setPosition(Vec2(110, 70));
    closeButton->addClickEventListener([this](Ref* sender) {
        this->hide();
        });
    this->addChild(closeButton);

    updateInfo();
}

void TownHallUpgradeUI::show() {
    if (!_building) return;

    updateInfo();
    this->setVisible(true);

    this->setScale(0.5f);
    auto scale = ScaleTo::create(0.2f, 1.0f);
    auto ease = EaseBackOut::create(scale);
    this->runAction(ease);
}

void TownHallUpgradeUI::hide() {
    // 先停止所有动作
    this->stopAllActions();

    auto scale = ScaleTo::create(0.15f, 0.5f);
    auto fade = FadeOut::create(0.15f);
    auto spawn = Spawn::create(scale, fade, nullptr);
    auto remove = CallFunc::create([this]() {
        // 重要：移除触摸监听器
        _eventDispatcher->removeEventListenersForTarget(this);
        // 从父节点移除
        this->removeFromParent();
        });
    this->runAction(Sequence::create(spawn, remove, nullptr));
}

bool TownHallUpgradeUI::isVisible() const {
    return Node::isVisible();
}

void TownHallUpgradeUI::setPositionNearBuilding(cocos2d::Node* building) {
    if (!building) return;

    Vec2 worldPos = building->getParent()->convertToWorldSpace(building->getPosition());
    worldPos.y += 100;

    this->setPosition(worldPos);
}

void TownHallUpgradeUI::onUpgradeClicked(cocos2d::Ref* sender) {
    if (!_building) return;

    bool success = _building->upgrade();

    if (success) {
        auto scaleUp = ScaleTo::create(0.2f, _building->getScale() * 1.3f);
        auto scaleDown = ScaleTo::create(0.2f, _building->getScale());
        auto flash = TintTo::create(0.1f, Color3B::YELLOW);
        auto normal = TintTo::create(0.1f, Color3B::WHITE);
        auto sequence = Sequence::create(
            Spawn::create(scaleUp, flash, nullptr),
            Spawn::create(scaleDown, normal, nullptr),
            nullptr
        );
        _building->runAction(sequence);

        updateInfo();

        if (_upgradeCallback) {
            _upgradeCallback(true, _building->getLevel());
        }

        if (!_building->canUpgrade()) {
            this->hide();
        }
    }
    else {
        auto shake = MoveBy::create(0.05f, Vec2(10, 0));
        auto shakeBack = MoveBy::create(0.05f, Vec2(-20, 0));
        auto shakeEnd = MoveBy::create(0.05f, Vec2(10, 0));
        auto red = TintTo::create(0.1f, Color3B::RED);
        auto normal = TintTo::create(0.1f, Color3B::WHITE);
        auto sequence = Sequence::create(
            Spawn::create(shake, red, nullptr),
            Spawn::create(shakeBack, normal, nullptr),
            shakeEnd,
            nullptr
        );
        _upgradeButton->runAction(sequence);

        if (_upgradeCallback) {
            _upgradeCallback(false, _building->getLevel());
        }
    }
}

void TownHallUpgradeUI::updateInfo() {
    if (!_building || !_infoLabel) return;

    _infoLabel->setString(_building->getUpgradeInfo());

    if (_building->canUpgrade()) {
        _upgradeButton->setEnabled(true);
        _upgradeButton->setColor(Color3B(100, 200, 100));
        _upgradeButton->setTitleText("升级");
    }
    else {
        _upgradeButton->setEnabled(false);
        _upgradeButton->setColor(Color3B(150, 150, 150));
        _upgradeButton->setTitleText("已满级");
    }
}

// ==================== ResourceDisplayUI 实现 ====================
ResourceDisplayUI* ResourceDisplayUI::create() {
    ResourceDisplayUI* ret = new (std::nothrow) ResourceDisplayUI();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ResourceDisplayUI::init() {
    if (!Node::init()) return false;

    setupResource(ResourceType::kGold, "💰", Color4B::YELLOW);
    setupResource(ResourceType::kElixir, "💧", Color4B(238, 130, 238, 255));
    setupResource(ResourceType::kGem, "💎", Color4B(0, 191, 255, 255));
    setupResource(ResourceType::kBuilder, "👷", Color4B::WHITE);

    setPositionAtTopLeft();

    ResourceManager::GetInstance()->SetOnResourceChangeCallback(
        [this](ResourceType type, int amount) {
            this->updateDisplay();
        }
    );

    updateDisplay();

    return true;
}

void ResourceDisplayUI::setupResource(ResourceType type, const std::string& icon, const cocos2d::Color4B& color) {
    ResourceDisplay display;
    display.container = Node::create();

    display.icon = Label::createWithSystemFont(icon, "Arial", 24);
    display.icon->setAnchorPoint(Vec2(0, 0.5f));
    display.icon->setPosition(Vec2(0, 0));
    display.container->addChild(display.icon);

    display.amount = Label::createWithSystemFont("0", "Arial", 20);
    display.amount->setAnchorPoint(Vec2(0, 0.5f));
    display.amount->setPosition(Vec2(40, 0));
    display.amount->setTextColor(color);
    display.container->addChild(display.amount);

    _displays[type] = display;
    this->addChild(display.container);
}

void ResourceDisplayUI::updateDisplay() {
    auto rm = ResourceManager::GetInstance();

    for (auto& pair : _displays) {
        ResourceType type = pair.first;
        ResourceDisplay& display = pair.second;

        int amount = rm->GetResourceCount(type);

        if (type == ResourceType::kBuilder) {
            int maxBuilders = rm->GetResourceCapacity(ResourceType::kBuilder);
            display.amount->setString(std::to_string(amount) + "/" + std::to_string(maxBuilders));
        }
        else {
            display.amount->setString(std::to_string(amount));
        }
    }
}

void ResourceDisplayUI::setPositionAtTopLeft() {
    auto visibleSize = Director::getInstance()->getVisibleSize();

    float startY = visibleSize.height - 50;
    float xPos = 30;

    int index = 0;
    for (auto& pair : _displays) {
        ResourceDisplay& display = pair.second;
        display.container->setPosition(Vec2(xPos, startY - index * 40));
        index++;
    }
}

void ResourceDisplayUI::setPositionAtTopRight() {
    auto visibleSize = Director::getInstance()->getVisibleSize();

    float startY = visibleSize.height - 50;
    float xPos = visibleSize.width - 150;

    int index = 0;
    for (auto& pair : _displays) {
        ResourceDisplay& display = pair.second;
        display.container->setPosition(Vec2(xPos, startY - index * 40));
        index++;
    }
}

void ResourceDisplayUI::setCustomPosition(const cocos2d::Vec2& position) {
    float startY = position.y;
    float xPos = position.x;

    int index = 0;
    for (auto& pair : _displays) {
        ResourceDisplay& display = pair.second;
        display.container->setPosition(Vec2(xPos, startY - index * 40));
        index++;
    }
}

void ResourceDisplayUI::showResource(ResourceType type, bool show) {
    auto it = _displays.find(type);
    if (it != _displays.end()) {
        it->second.container->setVisible(show);
    }
}