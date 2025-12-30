/****************************************************************
* Project Name:  Clash_of_Clans
* File Name:     DefenseBuilding.cpp
* File Function: 防御建筑实现
* Author:        薛毓哲
* Update Date:   2025/12/24
* License:       MIT License
****************************************************************/
#include "DefenseBuilding.h"

#include "UI/BuildingHealthBarUI.h"
#include "Unit/BaseUnit.h"

USING_NS_CC;

// 辅助函数
static bool isInRange(const Vec2& pos1, const Vec2& pos2, float range)
{
    return pos1.distance(pos2) <= range;
}

DefenseBuilding* DefenseBuilding::create(DefenseType defenseType, int level)
{
    DefenseBuilding* ret = new (std::nothrow) DefenseBuilding();
    if (ret && ret->init(defenseType, level))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

DefenseBuilding* DefenseBuilding::create(DefenseType defenseType, int level, const std::string& imageFile)
{
    DefenseBuilding* ret = new (std::nothrow) DefenseBuilding();
    if (ret && ret->init(defenseType, level, imageFile))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DefenseBuilding::init(DefenseType defenseType, int level)
{
    if (!BaseBuilding::init(level))
        return false;

    _type        = BuildingType::kDefense;  
    _defenseType = defenseType;
    _level       = level;

    initCombatStats();
    initHealthBarUI();

    return true;
}

bool DefenseBuilding::init(DefenseType defenseType, int level, const std::string& imageFile)
{
    if (!BaseBuilding::init(level, imageFile))
        return false;

    _type            = BuildingType::kDefense;  
    _defenseType     = defenseType;
    _customImagePath = imageFile;
    _level           = level;

    initCombatStats();
    initHealthBarUI();

    return true;
}

void DefenseBuilding::initCombatStats()
{
    switch (_defenseType)
    {
    case DefenseType::kCannon:
        _combatStats = DefenseConfig::getCannon(_level);
        break;
    case DefenseType::kArcherTower:
        _combatStats = DefenseConfig::getArcherTower(_level);
        break;
    case DefenseType::kWizardTower:
        _combatStats = DefenseConfig::getWizardTower(_level);
        break;
    default:
        _combatStats = DefenseConfig::getCannon(_level);
        break;
    }

    _maxHitpoints = _combatStats.maxHitpoints;
    _currentHitpoints = _maxHitpoints;
    _config.maxHitpoints = _maxHitpoints;
}

std::string DefenseBuilding::getDisplayName() const
{
    if (!_customName.empty())
        return _customName;

    switch (_defenseType)
    {
    case DefenseType::kCannon:
        return StringUtils::format("加农炮 (Lv.%d)", _level);
    case DefenseType::kArcherTower:
        return StringUtils::format("箭塔 (Lv.%d)", _level);
    case DefenseType::kWizardTower:
        return StringUtils::format("法师塔 (Lv.%d)", _level);
    default:
        return StringUtils::format("防御建筑 (Lv.%d)", _level);
    }
}

int DefenseBuilding::getUpgradeCost() const
{
    int baseCost = 0;

    switch (_defenseType)
    {
    case DefenseType::kCannon:
        baseCost = 250;
        break;
    case DefenseType::kArcherTower:
        baseCost = 500;
        break;
    case DefenseType::kWizardTower:
        baseCost = 1500;
        break;
    default:
        baseCost = 300;
        break;
    }

    return baseCost * _level;
}

float DefenseBuilding::getUpgradeTime() const
{
    return 20.0f + (_level * 10.0f);
}

int DefenseBuilding::getMaxLevel() const
{
    switch (_defenseType)
    {
    case DefenseType::kCannon:
    case DefenseType::kArcherTower:
        return 14;
    case DefenseType::kWizardTower:
        return 10;
    default:
        return 10;
    }
}

std::string DefenseBuilding::getBuildingDescription() const
{
    switch (_defenseType)
    {
    case DefenseType::kCannon:
        return "强大的地面防御建筑，对地面单位造成高额伤害。";
    case DefenseType::kArcherTower:
        return "可以攻击空中和地面目标的远程防御建筑。";
    case DefenseType::kWizardTower:
        return "释放强大的范围魔法攻击，对多个目标造成伤害。";
    default:
        return "防御建筑";
    }
}

std::string DefenseBuilding::getImageForLevel(int level) const
{
    // 始终根据等级生成路径，确保升级后图片正确更新
    switch (_defenseType)
    {
    case DefenseType::kCannon:
        return StringUtils::format("buildings/Cannon_Static/Cannon%d.png", level);
    case DefenseType::kArcherTower:
        return StringUtils::format("buildings/ArcherTower/Archer_Tower%d.png", level);
    case DefenseType::kWizardTower:
        return StringUtils::format("buildings/WizardTower/Wizard_Tower%d.png", level);
    default:
        return "";
    }
}

void DefenseBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    initCombatStats();
}

// ==================== 战斗逻辑 ====================

void DefenseBuilding::tick(float dt)
{
    if (!_battleModeEnabled || isDestroyed())
        return;

    if (_attackCooldown > 0.0f)
    {
        _attackCooldown -= dt;
    }

    if (_currentTarget)
    {
        // 检查目标是否死亡或等待移除
        if (_currentTarget->isDead() || _currentTarget->isPendingRemoval())
        {
            clearTarget();
            return;
        }

        Vec2 targetPos = _currentTarget->getPosition();
        Vec2 myPos     = this->getPosition();

        if (!isInRange(targetPos, myPos, _combatStats.attackRange))
        {
            clearTarget();
            return;
        }

        if (_attackCooldown <= 0.0f)
        {
            attackTarget(_currentTarget);
            _attackCooldown = _combatStats.attackSpeed;
        }
    }
}

void DefenseBuilding::detectEnemies(const std::vector<BaseUnit*>& units)
{
    if (!_battleModeEnabled || isDestroyed())
        return;

    // 如果已有有效目标，不重新选择
    if (_currentTarget && !_currentTarget->isDead() && !_currentTarget->isPendingRemoval())
        return;

    Vec2      myPos           = this->getPosition();
    BaseUnit* closestUnit     = nullptr;
    float     closestDistance = _combatStats.attackRange + 1.0f;

    for (auto* unit : units)
    {
        // 跳过空指针、已死亡或等待移除的单位
        if (!unit || unit->isDead() || unit->isPendingRemoval())
            continue;

        Vec2  unitPos  = unit->getPosition();
        float distance = myPos.distance(unitPos);

        if (distance <= _combatStats.attackRange && distance < closestDistance)
        {
            closestUnit     = unit;
            closestDistance = distance;
        }
    }

    if (closestUnit)
    {
        setTarget(closestUnit);
    }
}

void DefenseBuilding::attackTarget(BaseUnit* target)
{
    // 增加 isPendingRemoval 检查
    if (!target || target->isDead() || target->isPendingRemoval())
        return;

    BaseBuilding::attackTarget(target);

    fireProjectile(target);
    playAttackAnimation();
}

void DefenseBuilding::fireProjectile(BaseUnit* target)
{
    if (!target || target->isDead() || target->isPendingRemoval())
        return;

    // 保持对目标的引用，防止在动画过程中被释放
    target->retain();
    float damage = _combatStats.damage;

    // 创建炮弹/箭矢视觉效果
    Sprite* projectile      = nullptr;
    float   projectileSpeed = 0.0f;

    switch (_defenseType)
    {
    case DefenseType::kCannon:
        projectile      = createCannonballSprite();
        projectileSpeed = 600.0f;
        break;
    case DefenseType::kArcherTower:
        projectile      = createArrowSprite();
        projectileSpeed = 800.0f;
        break;
    case DefenseType::kWizardTower:
        projectile      = createCannonballSprite();
        projectileSpeed = 500.0f;
        break;
    default:
        projectile      = createCannonballSprite();
        projectileSpeed = 600.0f;
        break;
    }

    if (!projectile || !this->getParent())
    {
        // 创建失败时直接造成伤害
        if (!target->isDead() && !target->isPendingRemoval())
        {
            target->takeDamage(damage);
        }
        target->release();
        return;
    }

    // 炮弹飞行动画
    Vec2 startPos = this->getPosition();
    Vec2 endPos   = target->getPosition();

    projectile->setPosition(startPos);
    this->getParent()->addChild(projectile, 5000);

    float distance = startPos.distance(endPos);
    float duration = distance / projectileSpeed;

    // 箭矢旋转朝向目标
    if (_defenseType == DefenseType::kArcherTower)
    {
        Vec2  direction = endPos - startPos;
        float angle     = CC_RADIANS_TO_DEGREES(direction.getAngle());
        projectile->setRotation(-angle);
    }

    auto moveTo = MoveTo::create(duration, endPos);
    
    // 在回调中使用 retain/release 管理目标生命周期
    auto hitCallback = CallFunc::create([target, damage, projectile]() {
        // 确保目标仍然存在且未死亡
        if (!target->isDead() && !target->isPendingRemoval())
        {
            target->takeDamage(damage);
        }
        // 释放之前 retain 的引用
        target->release();
        projectile->removeFromParent();
    });

    auto sequence = Sequence::create(moveTo, hitCallback, nullptr);
    projectile->runAction(sequence);
}

void DefenseBuilding::playAttackAnimation()
{
    auto scaleUp   = ScaleTo::create(0.1f, 1.1f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f);
    auto seq       = Sequence::create(scaleUp, scaleDown, nullptr);
    this->runAction(seq);
}

// ==================== 攻击范围显示 ====================

void DefenseBuilding::showAttackRange()
{
    if (_rangeCircle)
    {
        _rangeCircle->setVisible(true);
        return;
    }

    _rangeCircle = DrawNode::create();

    // 根据建筑类型选择不同的颜色
    Color4F circleColor;
    switch (_defenseType)
    {
    case DefenseType::kCannon:
        circleColor = Color4F(1.0f, 0.0f, 0.0f, 0.3f);
        break;
    case DefenseType::kArcherTower:
        circleColor = Color4F(0.0f, 1.0f, 0.0f, 0.3f);
        break;
    case DefenseType::kWizardTower:
        circleColor = Color4F(0.5f, 0.0f, 1.0f, 0.3f);
        break;
    default:
        circleColor = Color4F(1.0f, 1.0f, 0.0f, 0.3f);
        break;
    }

    _rangeCircle->drawCircle(Vec2::ZERO, _combatStats.attackRange, 0, 100, false, 2.0f, 2.0f, circleColor);
    this->addChild(_rangeCircle, -1);
}

void DefenseBuilding::hideAttackRange()
{
    if (_rangeCircle)
    {
        _rangeCircle->setVisible(false);
    }
}

void DefenseBuilding::rotateToTarget(const cocos2d::Vec2& targetPos)
{
    // 已废弃：防御建筑保持静止，只有投射物朝向目标
}

// ==================== 炮弹/箭矢创建 ====================

Sprite* DefenseBuilding::createCannonballSprite()
{
    auto cannonball = Sprite::create();
    if (!cannonball)
    {
        auto drawNode = DrawNode::create();
        drawNode->drawSolidCircle(Vec2::ZERO, 8.0f, 0, 20, Color4F::BLACK);
        return (Sprite*)drawNode;
    }
    cannonball->setScale(0.5f);
    return cannonball;
}

Sprite* DefenseBuilding::createArrowSprite()
{
    auto arrow = Sprite::create();
    if (!arrow)
    {
        auto drawNode      = DrawNode::create();
        Vec2 arrowPoints[] = { Vec2(-15, 0), Vec2(15, 0) };
        drawNode->drawSegment(arrowPoints[0], arrowPoints[1], 2.0f, Color4F(0.6f, 0.3f, 0.0f, 1.0f));
        return (Sprite*)drawNode;
    }
    return arrow;
}
