//
#include "ArmySelectionUI.h"
#include "Managers/TroopInventory.h" // 🆕 引入库存管理器

USING_NS_CC;
using namespace ui;

ArmySelectionUI* ArmySelectionUI::create()
{
    ArmySelectionUI* ret = new (std::nothrow) ArmySelectionUI();
    // 由于需要先加载数据，我们在这里调用 init
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ArmySelectionUI::init()
{
    if (!Layer::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    // 1. 获取现有库存
    _availableTroops = TroopInventory::getInstance().getAllTroops();

    // 2. 初始化选择数量为 0
    for (const auto& pair : _availableTroops)
    {
        _selectedTroops[pair.first] = 0;
    }

    // 3. UI 初始化

    // 半透明背景遮罩
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(bgMask);

    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    createUI();

    return true;
}

// 🆕 修改回调签名
void ArmySelectionUI::setOnConfirmed(std::function<void(const TroopDeploymentMap&)> callback)
{
    _onConfirmed = callback;
}

void ArmySelectionUI::setOnCancelled(std::function<void()> callback)
{
    _onCancelled = callback;
}

void ArmySelectionUI::createUI()
{
    // 创建容器
    auto layout = ui::Layout::create();
    // 容器尺寸调整以适应列表
    layout->setContentSize(Size(650, 450));
    layout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    layout->setBackGroundColor(Color3B(40, 40, 50));
    layout->setBackGroundColorOpacity(255);

    _container = layout;
    _container->setAnchorPoint(Vec2(0.5f, 0.5f));
    _container->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height / 2));
    this->addChild(_container);

    // 标题
    auto title = Label::createWithSystemFont("选择你的军队", "Microsoft YaHei", 32);
    title->setPosition(Vec2(325, 410));
    title->setTextColor(Color4B::YELLOW);
    _container->addChild(title);

    // --- 兵种列表容器 ---
    auto listContainer = ui::Layout::create();
    listContainer->setContentSize(Size(600, 300));
    listContainer->setAnchorPoint(Vec2(0.5f, 0.5f));
    listContainer->setPosition(Vec2(325, 230));
    listContainer->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    listContainer->setBackGroundColor(Color3B(50, 50, 60));
    listContainer->setBackGroundColorOpacity(255);
    _container->addChild(listContainer);

    // 绘制兵种卡片
    int cardIndex = 0;
    // 按照 UnitType 的顺序遍历 (Barbarian=0, Archer=1, Giant=2, Goblin=3, WallBreaker=4)
    for (const auto& pair : _availableTroops)
    {
        // 只有库存大于 0 的才显示在选择列表
        if (pair.second > 0)
        {
            createTroopCard(listContainer, pair.first, cardIndex);
            cardIndex++;
        }
    }

    // 确认按钮
    _confirmBtn = Button::create();
    _confirmBtn->setTitleText("开始攻击！");
    _confirmBtn->setTitleFontSize(28);
    _confirmBtn->setContentSize(Size(180, 60));
    _confirmBtn->setScale9Enabled(true);
    _confirmBtn->setPosition(Vec2(450, 60));

    _confirmBtn->addClickEventListener([this](Ref*) {
        if (_onConfirmed)
        {
            // 部署时传递最终选择的军队 Map
            _onConfirmed(_selectedTroops);
        }
        hide();
        });
    _container->addChild(_confirmBtn);

    // 取消按钮
    _cancelBtn = Button::create();
    _cancelBtn->setTitleText("取消");
    _cancelBtn->setTitleFontSize(24);
    _cancelBtn->setContentSize(Size(140, 60));
    _cancelBtn->setScale9Enabled(true);
    _cancelBtn->setPosition(Vec2(200, 60));
    _cancelBtn->addClickEventListener([this](Ref*) {
        if (_onCancelled)
        {
            _onCancelled();
        }
        hide();
        });
    _container->addChild(_cancelBtn);
}


// --------------------------------------------------------------------------
// 🆕 兵种卡片创建函数
// --------------------------------------------------------------------------
void ArmySelectionUI::createTroopCard(cocos2d::ui::Layout* parent, UnitType type, int cardIndex)
{
    float cardHeight = 60.0f;
    float totalHeight = parent->getContentSize().height;

    // 计算 Y 轴位置 (从上往下排列)
    float posY = totalHeight - (cardIndex * cardHeight) - (cardHeight / 2.0f);

    // 创建卡片行
    auto cardLayout = Layout::create();
    cardLayout->setContentSize(Size(580, cardHeight));
    cardLayout->setPosition(Vec2(10, posY - 10.0f)); // 略微向下偏移以适应排版
    cardLayout->setAnchorPoint(Vec2(0.0f, 0.5f));
    parent->addChild(cardLayout);

    // --- 1. 兵种图标 (左侧) ---
    auto icon = Sprite::create(getUnitIconPath(type));
    if (icon)
    {
        icon->setScale(0.5f);
        icon->setPosition(Vec2(40, cardHeight / 2.0f));
        cardLayout->addChild(icon);
    }

    // --- 2. 名称 (左侧) ---
    auto nameLabel = Label::createWithSystemFont(getUnitName(type), "Microsoft YaHei", 20);
    nameLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    nameLabel->setPosition(Vec2(80, cardHeight / 2.0f));
    nameLabel->setTextColor(Color4B::WHITE);
    cardLayout->addChild(nameLabel);

    // --- 3. 数量显示 (中间) ---
    int availableCount = _availableTroops.at(type);

    // 现有数量 / 总数量标签
    auto totalCountLabel = Label::createWithSystemFont(
        StringUtils::format("库存: %d", availableCount),
        "Microsoft YaHei", 16);
    totalCountLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    totalCountLabel->setPosition(Vec2(220, cardHeight / 2.0f));
    totalCountLabel->setTextColor(Color4B(200, 200, 255, 255));
    cardLayout->addChild(totalCountLabel);

    // --- 4. 选择数量 (右侧) ---

    // 减号按钮
    auto minusBtn = Button::create("icon/minus_button.png"); // 假设有资源
    minusBtn->setScale(0.5f);
    minusBtn->setPosition(Vec2(400, cardHeight / 2.0f));
    minusBtn->addClickEventListener([this, type](Ref*) {
        onTroopCountChanged(type, -1);
        });
    cardLayout->addChild(minusBtn);

    // 选中数量 Label
    auto countLabel = Label::createWithSystemFont("0", "Microsoft YaHei", 24);
    countLabel->setPosition(Vec2(460, cardHeight / 2.0f));
    countLabel->setTextColor(Color4B::GREEN);
    cardLayout->addChild(countLabel);
    _countLabels[type] = countLabel; // 保存引用

    // 加号按钮
    auto plusBtn = Button::create("icon/plus_button.png"); // 假设有资源
    plusBtn->setScale(0.5f);
    plusBtn->setPosition(Vec2(520, cardHeight / 2.0f));
    plusBtn->addClickEventListener([this, type](Ref*) {
        onTroopCountChanged(type, 1);
        });
    cardLayout->addChild(plusBtn);
}


// --------------------------------------------------------------------------
// 🆕 加减号点击事件处理
// --------------------------------------------------------------------------
void ArmySelectionUI::onTroopCountChanged(UnitType type, int delta)
{
    // 获取当前选择数量和最大库存
    int currentSelected = _selectedTroops.at(type);
    int maxAvailable = _availableTroops.at(type);

    int newCount = currentSelected + delta;

    // 边界检查：不能低于 0
    newCount = std::max(0, newCount);
    // 边界检查：不能超过库存
    newCount = std::min(maxAvailable, newCount);

    if (newCount != currentSelected)
    {
        _selectedTroops[type] = newCount;

        // 更新 UI
        if (_countLabels.count(type))
        {
            _countLabels.at(type)->setString(std::to_string(newCount));
        }

        CCLOG("Selected %s count: %d", getUnitName(type).c_str(), newCount);
    }
}


// --------------------------------------------------------------------------
// 辅助函数
// --------------------------------------------------------------------------

void ArmySelectionUI::show()
{
    this->setVisible(true);
    _container->setScale(0.0f);
    _container->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
    // 每次显示时，重置选择数量为 0 (或者保留上次选择，取决于产品需求，这里选择重置)
    for (auto& pair : _selectedTroops)
    {
        pair.second = 0;
        if (_countLabels.count(pair.first))
        {
            _countLabels.at(pair.first)->setString("0");
        }
    }
}

void ArmySelectionUI::hide()
{
    auto scaleAction = ScaleTo::create(0.2f, 0.0f);
    _container->runAction(scaleAction);

    auto delay = DelayTime::create(0.2f);
    auto removeSelf = RemoveSelf::create();
    this->runAction(Sequence::create(delay, removeSelf, nullptr));
}

std::string ArmySelectionUI::getUnitName(UnitType type) const
{
    switch (type)
    {
    case UnitType::kBarbarian: return "野蛮人";
    case UnitType::kArcher: return "弓箭手";
    case UnitType::kGiant: return "巨人";
    case UnitType::kGoblin: return "哥布林";
    case UnitType::kWallBreaker: return "炸弹人";
    default: return "未知";
    }
}

std::string ArmySelectionUI::getUnitIconPath(UnitType type) const
{
    // 假设这些资源是可用的，您可能需要调整路径或文件名以匹配您的项目
    switch (type)
    {
    case UnitType::kBarbarian: return "units/barbarian_select_button_active.png";
    case UnitType::kArcher: return "units/archer_select_button_active.png";
    case UnitType::kGiant: return "units/giant_select_button_active.png";
    case UnitType::kGoblin: return "units/goblin_select_button_active.png";
    case UnitType::kWallBreaker: return "units/wallbreaker_select_button_active.png";
    default: return "icon/default_unit.png"; // 默认占位符
    }
}