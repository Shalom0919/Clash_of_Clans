/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeUI.cpp
 * File Function: 通用建筑升级界面实现
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#include "BuildingUpgradeUI.h"
#include "ResourceManager.h"
#include "ArmyBuilding.h"
#include "Unit/TrainingUI.h"

USING_NS_CC;
using namespace ui;

BuildingUpgradeUI* BuildingUpgradeUI::create(BaseBuilding* building)
{
    BuildingUpgradeUI* ui = new (std::nothrow) BuildingUpgradeUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool BuildingUpgradeUI::init(BaseBuilding* building)
{
    if (!Node::init())
    {
        return false;
    }

    _building = building;
    setupUI();
    updateUI();
    return true;
}

void BuildingUpgradeUI::setupUI()
{
    // 检查是否为兵营，如果是则面板高度增加
    bool isBarracks = (_building && _building->getBuildingType() == BuildingType::kArmy);
    float panelHeight = isBarracks ? 270.0f : 220.0f;  // 兵营面板更高
    
    // 背景面板
    _panel = Layout::create();
    _panel->setContentSize(Size(280, panelHeight));
    _panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _panel->setBackGroundColor(Color3B(40, 40, 60));
    _panel->setBackGroundColorOpacity(230);
    _panel->setAnchorPoint(Vec2(0.5f, 0.0f));
    this->addChild(_panel);

    float panelWidth = _panel->getContentSize().width;

    // 标题
    _titleLabel = Label::createWithSystemFont("", "Microsoft YaHei", 20);
    _titleLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 25));
    _titleLabel->setTextColor(Color4B::YELLOW);
    _panel->addChild(_titleLabel);

    // 等级
    _levelLabel = Label::createWithSystemFont("", "Microsoft YaHei", 16);
    _levelLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 50));
    _levelLabel->setTextColor(Color4B::WHITE);
    _panel->addChild(_levelLabel);

    // 描述
    _descLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _descLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 85));
    _descLabel->setTextColor(Color4B(200, 200, 200, 255));
    _descLabel->setAlignment(TextHAlignment::CENTER);
    _panel->addChild(_descLabel);

    // 费用
    _costLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _costLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 120));
    _costLabel->setTextColor(Color4B::GREEN);
    _panel->addChild(_costLabel);

    // 时间
    _timeLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _timeLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 145));
    _timeLabel->setTextColor(Color4B(150, 200, 255, 255));
    _panel->addChild(_timeLabel);

    // ========== 按钮布局（根据是否为兵营调整）==========
    if (isBarracks)
    {
        // 兵营：三个按钮（训练、升级、关闭）
        
        // 训练按钮（顶部）
        _trainButton = Button::create();
        _trainButton->setTitleText("🎖 训练士兵");
        _trainButton->setTitleFontSize(18);
        _trainButton->setContentSize(Size(200, 40));
        _trainButton->setPosition(Vec2(panelWidth / 2, 85));
        _trainButton->addClickEventListener([this](Ref*) { onTrainClicked(); });
        
        auto trainBg = LayerColor::create(Color4B(50, 100, 200, 200), 200, 40);
        trainBg->setPosition(Vec2(-100, -20));
        _trainButton->addChild(trainBg, -1);
        _panel->addChild(_trainButton);
        
        // 升级按钮（左下）
        _upgradeButton = Button::create();
        _upgradeButton->setTitleText("升级");
        _upgradeButton->setTitleFontSize(18);
        _upgradeButton->setContentSize(Size(100, 40));
        _upgradeButton->setPosition(Vec2(panelWidth / 2 - 60, 35));
        _upgradeButton->addClickEventListener([this](Ref*) { onUpgradeClicked(); });

        auto upgradeBg = LayerColor::create(Color4B(0, 150, 0, 200), 100, 40);
        upgradeBg->setPosition(Vec2(-50, -20));
        _upgradeButton->addChild(upgradeBg, -1);
        _panel->addChild(_upgradeButton);

        // 关闭按钮（右下）
        _closeButton = Button::create();
        _closeButton->setTitleText("关闭");
        _closeButton->setTitleFontSize(18);
        _closeButton->setContentSize(Size(100, 40));
        _closeButton->setPosition(Vec2(panelWidth / 2 + 60, 35));
        _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });

        auto closeBg = LayerColor::create(Color4B(150, 0, 0, 200), 100, 40);
        closeBg->setPosition(Vec2(-50, -20));
        _closeButton->addChild(closeBg, -1);
        _panel->addChild(_closeButton);
    }
    else
    {
        // 普通建筑：两个按钮（升级、关闭）
        
        // 升级按钮
        _upgradeButton = Button::create();
        _upgradeButton->setTitleText("升级");
        _upgradeButton->setTitleFontSize(18);
        _upgradeButton->setContentSize(Size(100, 40));
        _upgradeButton->setPosition(Vec2(panelWidth / 2 - 60, 35));
        _upgradeButton->addClickEventListener([this](Ref*) { onUpgradeClicked(); });

        auto upgradeBg = LayerColor::create(Color4B(0, 150, 0, 200), 100, 40);
        upgradeBg->setPosition(Vec2(-50, -20));
        _upgradeButton->addChild(upgradeBg, -1);
        _panel->addChild(_upgradeButton);

        // 关闭按钮
        _closeButton = Button::create();
        _closeButton->setTitleText("关闭");
        _closeButton->setTitleFontSize(18);
        _closeButton->setContentSize(Size(100, 40));
        _closeButton->setPosition(Vec2(panelWidth / 2 + 60, 35));
        _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });

        auto closeBg = LayerColor::create(Color4B(150, 0, 0, 200), 100, 40);
        closeBg->setPosition(Vec2(-50, -20));
        _closeButton->addChild(closeBg, -1);
        _panel->addChild(_closeButton);
    }
}

void BuildingUpgradeUI::updateUI()
{
    if (!_building)
        return;

    _titleLabel->setString(_building->getDisplayName());

    if (_building->isMaxLevel())
    {
        _levelLabel->setString(StringUtils::format("等级: %d (MAX)", _building->getLevel()));
        _costLabel->setString("已达最高等级");
        _timeLabel->setString("");
        _upgradeButton->setEnabled(false);
        _upgradeButton->setTitleColor(Color3B::GRAY);
    }
    else
    {
        _levelLabel->setString(StringUtils::format("等级: %d → %d",
                                                   _building->getLevel(),
                                                   _building->getLevel() + 1));

        std::string resName = getResourceTypeName(_building->getUpgradeCostType());
        _costLabel->setString(StringUtils::format("费用: %d %s",
                                                  _building->getUpgradeCost(),
                                                  resName.c_str()));

        _timeLabel->setString(StringUtils::format("时间: %s",
                                                  formatTime(_building->getUpgradeTime()).c_str()));

        bool canUpgrade = _building->canUpgrade();
        _upgradeButton->setEnabled(canUpgrade);
        _upgradeButton->setTitleColor(canUpgrade ? Color3B::WHITE : Color3B::GRAY);
        _costLabel->setTextColor(canUpgrade ? Color4B::GREEN : Color4B::RED);
    }

    _descLabel->setString(_building->getBuildingDescription());
}

void BuildingUpgradeUI::setPositionNearBuilding(BaseBuilding* building)
{
    if (!building)
        return;

    Vec2 worldPos = building->getParent()->convertToWorldSpace(building->getPosition());
    Vec2 offset(0, building->getContentSize().height * building->getScale() * 0.5f + 20);
    this->setPosition(worldPos + offset);
}

void BuildingUpgradeUI::show()
{
    this->setVisible(true);
    this->setScale(0.0f);
    auto scaleIn = EaseBackOut::create(ScaleTo::create(0.2f, 1.0f));
    this->runAction(scaleIn);
}

void BuildingUpgradeUI::hide()
{
    auto scaleOut = EaseBackIn::create(ScaleTo::create(0.15f, 0.0f));
    auto remove = CallFunc::create([this]() {
        this->removeFromParent();
    });
    this->runAction(Sequence::create(scaleOut, remove, nullptr));
}

void BuildingUpgradeUI::onUpgradeClicked()
{
    if (!_building)
        return;

    bool success = _building->upgrade();

    if (_resultCallback)
    {
        _resultCallback(success, _building->getLevel());
    }

    if (success)
    {
        updateUI();
    }
}

void BuildingUpgradeUI::onCloseClicked()
{
    if (_closeCallback) _closeCallback(); // 通知拥有者先清理引用
    hide();
}

void BuildingUpgradeUI::onTrainClicked()
{
    // 检查是否为兵营建筑
    if (!_building || _building->getBuildingType() != BuildingType::kArmy)
        return;
    
    // 转换为兵营类型
    auto barracks = dynamic_cast<ArmyBuilding*>(_building);
    if (!barracks)
        return;
    
    CCLOG("打开训练UI：%s", barracks->getDisplayName().c_str());
    
    // 先关闭当前升级UI
    if (_closeCallback) _closeCallback();
    
    // 创建训练UI
    auto trainingUI = TrainingUI::create(barracks);
    if (trainingUI)
    {
        // 获取场景根节点
        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
        {
            scene->addChild(trainingUI, 2000);  // 高层级显示
        }
    }
    
    // 隐藏升级UI
    hide();
}

std::string BuildingUpgradeUI::getResourceTypeName(ResourceType type) const
{
    switch (type)
    {
        case ResourceType::kGold:
            return "金币";
        case ResourceType::kElixir:
            return "圣水";
        case ResourceType::kGem:
            return "宝石";
        default:
            return "未知";
    }
}

std::string BuildingUpgradeUI::formatTime(int seconds) const
{
    if (seconds < 60)
    {
        return StringUtils::format("%d秒", seconds);
    }
    else if (seconds < 3600)
    {
        return StringUtils::format("%d分钟", seconds / 60);
    }
    else if (seconds < 86400)
    {
        return StringUtils::format("%d小时%d分", seconds / 3600, (seconds % 3600) / 60);
    }
    else
    {
        return StringUtils::format("%d天%d小时", seconds / 86400, (seconds % 86400) / 3600);
    }
}