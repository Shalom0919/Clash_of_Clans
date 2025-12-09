/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function: 建筑倒计时类
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#include "UpgradeTimerUI.h"
#include "Buildings/BaseBuilding.h"

USING_NS_CC;

UpgradeTimerUI* UpgradeTimerUI::create(BaseBuilding* building)
{
    UpgradeTimerUI* ret = new (std::nothrow) UpgradeTimerUI();
    if (ret && ret->init(building))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool UpgradeTimerUI::init(BaseBuilding* building)
{
    if (!Node::init()) return false;
    _building = building;

    // 配置参数
    const Size barSize(80, 10); // 进度条尺寸
    const Color3B bgColor(50, 50, 50);   // 深灰色背景
    const Color3B fillColor(0, 255, 0);  // 亮绿色填充

    // 1. 创建背景条 (灰色)
    _barBg = createColorSprite(barSize, bgColor);
    _barBg->setPosition(Vec2(0, 100)); // 在建筑上方 100 像素
    this->addChild(_barBg);

    // 2. 创建填充条 (绿色)
    _barFill = createColorSprite(barSize, fillColor);
    // 关键：设置锚点为 (0, 0.5)，这样 setScaleX 会从左向右延伸
    _barFill->setAnchorPoint(Vec2(0.0f, 0.5f));
    // 位置设为背景的左边缘
    _barFill->setPosition(Vec2(-barSize.width / 2, 100));
    _barFill->setScaleX(0.0f); // 初始进度为 0
    this->addChild(_barFill);

    // 3. 创建时间标签
    _timeLabel = Label::createWithSystemFont("00:00", "Arial", 16);
    _timeLabel->setPosition(Vec2(0, 120)); // 在进度条上方
    _timeLabel->enableOutline(Color4B::BLACK, 2); // 黑色描边，防止看不清
    this->addChild(_timeLabel);

    scheduleUpdate();
    return true;
}

void UpgradeTimerUI::update(float dt)
{
    if (!_building) return;

    // 如果建筑不再升级，销毁自己
    if (!_building->isUpgrading())
    {
        this->removeFromParent();
        return;
    }

    // 获取数据
    float progress = _building->getUpgradeProgress(); // 0.0 ~ 1.0
    float remaining = _building->getUpgradeRemainingTime();

    // 更新 UI
    if (_barFill)
    {
        // 限制进度在 0~1 之间
        progress = std::max(0.0f, std::min(1.0f, progress));
        _barFill->setScaleX(progress);
    }

    if (_timeLabel)
    {
        _timeLabel->setString(formatTime(remaining));
    }
}

cocos2d::Sprite* UpgradeTimerUI::createColorSprite(const cocos2d::Size& size, const cocos2d::Color3B& color)
{
    auto sprite = Sprite::create();
    sprite->setTextureRect(Rect(0, 0, size.width, size.height));
    sprite->setColor(color);
    return sprite;
}

std::string UpgradeTimerUI::formatTime(float seconds)
{
    int s = static_cast<int>(ceil(seconds)); // 向上取整
    if (s < 0) s = 0;
    int m = s / 60;
    s %= 60;
    // 格式化为 00:00
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return std::string(buf);
}