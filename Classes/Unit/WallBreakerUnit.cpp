/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBreakerUnit.cpp
 * File Function: 炸弹人单位类实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/22
 * License:       MIT License
 ****************************************************************/
#include "WallBreakerUnit.h"
#include "Unit/UnitConfig.h"

USING_NS_CC;

WallBreakerUnit* WallBreakerUnit::create(int level)
{
    WallBreakerUnit* unit = new (std::nothrow) WallBreakerUnit();
    if (unit && unit->init(level))
    {
        unit->autorelease();
        return unit;
    }
    CC_SAFE_DELETE(unit);
    return nullptr;
}

bool WallBreakerUnit::init(int level)
{
    if (!BaseUnit::init(level))
        return false;

    // 设置炸弹人特有属性
    _moveSpeed = 120.0f;
    _combatStats = UnitConfig::getWallBreaker(level);

    return true;
}

void WallBreakerUnit::loadAnimations()
{
    // 加载图集
    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("units/wall_breaker/wall_breaker.plist");

    // 创建精灵
    _sprite = Sprite::createWithSpriteFrameName("wall_breaker21.0.png");
    if (_sprite)
    {
        this->addChild(_sprite);
        _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
    }

    // 加载跑步动画
    addAnimFromFrames("wall_breaker", "run_down_right", 49, 56, 0.10f);
    addAnimFromFrames("wall_breaker", "run_right", 41, 48, 0.10f);
    addAnimFromFrames("wall_breaker", "run_up_right", 33, 40, 0.10f);

    // 加载待机动画
    addAnimFromFrames("wall_breaker", "idle_down_right", 27, 27, 1.0f);
    addAnimFromFrames("wall_breaker", "idle_right", 21, 21, 1.0f);
    addAnimFromFrames("wall_breaker", "idle_up_right", 20, 20, 1.0f);

    // 加载死亡动画（爆炸）
    addAnimFromFrames("wall_breaker", "death", 2, 1, 0.5f);

    // 播放待机动画
    playAnimation(UnitAction::kIdle, UnitDirection::kRight);
}

void WallBreakerUnit::onDeathBefore()
{
    // 炸弹人特殊死亡逻辑：爆炸后直接消失（不留墓碑）
    playAnimation(UnitAction::kDeath, _currentDir);
    
    auto removeAction = Sequence::create(
        DelayTime::create(0.5f),  // 等待爆炸动画播完
        RemoveSelf::create(),
        nullptr
    );
    this->runAction(removeAction);
    
    CCLOG("💣 炸弹人爆炸！");
}