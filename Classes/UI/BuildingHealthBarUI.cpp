/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingHealthBarUI.cpp
 * File Function: 建筑血条UI显示组件实现
 * Author:        刘相成
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#include "BuildingHealthBarUI.h"

USING_NS_CC;

BuildingHealthBarUI* BuildingHealthBarUI::create(BaseBuilding* building)
{
    BuildingHealthBarUI* ui = new (std::nothrow) BuildingHealthBarUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool BuildingHealthBarUI::init(BaseBuilding* building)
{
    if (!Node::init() || !building)
    {
        return false;
    }

    _building        = building;
    _lastHealthValue = building->getHitpoints();

    // ==================== 创建血条背景（红色 - 已损伤部分） ====================
    _healthBarBg = LayerColor::create(Color4B(200, 50, 50, 255), BAR_WIDTH, BAR_HEIGHT);
    _healthBarBg->setPosition(Vec2(-BAR_WIDTH / 2.0f, OFFSET_Y));
    _healthBarBg->setAnchorPoint(Vec2(0.0f, 0.5f));
    this->addChild(_healthBarBg, 1);

    // ==================== 创建血条填充（绿色 - 剩余生命值） ====================
    _healthBarFill = LayerColor::create(Color4B(50, 200, 50, 255), BAR_WIDTH, BAR_HEIGHT);
    _healthBarFill->setPosition(Vec2(-BAR_WIDTH / 2.0f, OFFSET_Y));
    _healthBarFill->setAnchorPoint(Vec2(0.0f, 0.5f));
    this->addChild(_healthBarFill, 2);

    // ==================== 创建血量文字标签 ====================
    int currentHP = building->getHitpoints();
    int maxHP     = building->getMaxHitpoints();
    _healthLabel  = Label::createWithSystemFont(StringUtils::format("%d/%d", currentHP, maxHP), "Arial", 14);
    _healthLabel->setPosition(Vec2(0.0f, OFFSET_Y + 15.0f));
    _healthLabel->setTextColor(Color4B::WHITE);
    this->addChild(_healthLabel, 3);

    // ==================== 初始状态设置 ====================
    // 血量满时不显示血条，受伤时才显示
    if (currentHP >= maxHP)
    {
        this->setVisible(false);
        _isVisible = false;
    }
    else
    {
        this->setVisible(true);
        _isVisible = true;
    }

    // 启用每帧更新
    this->scheduleUpdate();

    return true;
}

void BuildingHealthBarUI::update(float dt)
{
    if (!_building || isBuildingDestroyed())
    {
        this->removeFromParent();
        return;
    }

    int currentHP = _building->getHitpoints();
    int maxHP     = _building->getMaxHitpoints();

    // ==================== 检测生命值变化 ====================
    if (currentHP != _lastHealthValue)
    {
        _lastHealthValue = currentHP;
        _hideTimer       = 0.0f; // 重置隐藏计时器

        // 显示血条
        if (!_isVisible && !_alwaysVisible)
        {
            this->setVisible(true);
            _isVisible = true;

            // 播放血条出现动画
            this->setOpacity(0);
            auto fadeIn = FadeIn::create(0.2f);
            this->runAction(fadeIn);
        }

        // ==================== 更新血条填充宽度 ====================
        if (maxHP > 0)
        {
            float healthPercent = static_cast<float>(currentHP) / maxHP;
            _healthBarFill->setContentSize(Size(BAR_WIDTH * healthPercent, BAR_HEIGHT));

            // 根据生命值百分比改变血条颜色
            if (healthPercent > 0.5f)
            {
                // 绿色：血量充足
                _healthBarFill->setColor(Color3B(50, 200, 50));
            }
            else if (healthPercent > 0.25f)
            {
                // 黄色：血量不足
                _healthBarFill->setColor(Color3B(255, 200, 50));
            }
            else
            {
                // 红色：血量严重不足
                _healthBarFill->setColor(Color3B(255, 50, 50));
            }

            CCLOG("🩹 %s 受伤！HP: %d/%d (%.1f%%)", _building->getDisplayName().c_str(), currentHP, maxHP,
                  healthPercent * 100);
        }

        // ==================== 更新血量文字 ====================
        _healthLabel->setString(StringUtils::format("%d/%d", currentHP, maxHP));
    }

    // ==================== 血量恢复满后自动隐藏（不是战斗状态时） ====================
    if (!_alwaysVisible && currentHP >= maxHP)
    {
        _hideTimer += dt;

        if (_hideTimer >= HIDE_DELAY && _isVisible)
        {
            // 播放血条消失动画
            auto fadeOut  = FadeOut::create(0.3f);
            auto callback = CallFunc::create([this]() {
                this->setVisible(false);
                _isVisible = false;
            });
            this->runAction(Sequence::create(fadeOut, callback, nullptr));
        }
    }
}

void BuildingHealthBarUI::show()
{
    if (!_isVisible)
    {
        this->setVisible(true);
        _isVisible = true;
        this->setOpacity(255);
    }
}

void BuildingHealthBarUI::hide()
{
    if (_isVisible)
    {
        this->setVisible(false);
        _isVisible = false;
    }
}

bool BuildingHealthBarUI::isBuildingDestroyed() const
{
    if (!_building)
    {
        return true;
    }

    return _building->isDestroyed();
}