/**
 * @file ResourceBuilding.cpp
 * @brief 资源生产建筑实现
 */
#include "ResourceBuilding.h"
USING_NS_CC;
static const int GOLD_PRODUCTION_RATES[] = {0,    200,  400,  600,  800,  1000, 1200, 1400,
                                            1600, 1800, 2000, 2200, 2400, 2600, 2800, 3000};
static const int STORAGE_CAPACITIES[] = {0,    500,   1000,  1500,  2000,  3000,  4000,  5000,
                                         7500, 10000, 15000, 20000, 30000, 50000, 75000, 100000};
static const int UPGRADE_COSTS[] = {0,     150,   300,    700,    1400,   3000,   7000,    14000,
                                    28000, 56000, 100000, 200000, 400000, 800000, 1500000, 0};
ResourceBuilding* ResourceBuilding::create(ResourceType resourceType, int level)
{
    ResourceBuilding* ret = new (std::nothrow) ResourceBuilding();
    if (ret)
    {
        ret->_resourceType = resourceType;
        if (ret->init(level))
        {
            ret->autorelease();
            return ret;
        }
    }
    delete ret;
    return nullptr;
}
bool ResourceBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(3, 3);
    _currentStorage = 0;
    _productionAccumulator = 0.0f;
    std::string imageFile = getImageFile();
    if (!Sprite::initWithFile(imageFile))
        return false;
    this->setAnchorPoint(Vec2(0.5f, 0.2f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    _storageLabel = Label::createWithSystemFont("0", "Arial", 12);
    _storageLabel->setPosition(Vec2(this->getContentSize().width / 2, -10));
    _storageLabel->setTextColor(Color4B::WHITE);
    _storageLabel->setVisible(false);
    this->addChild(_storageLabel, 100);
    return true;
}
std::string ResourceBuilding::getDisplayName() const
{
    std::string typeName;
    switch (_resourceType)
    {
    case kGold:
        typeName = "金矿";
        break;
    case kElixir:
        typeName = "圣水收集器";
        break;
    default:
        typeName = "资源建筑";
        break;
    }
    return typeName + " Lv." + std::to_string(_level);
}
std::string ResourceBuilding::getImageFile() const
{
    switch (_resourceType)
    {
    case kGold:
        return "GoldMine.png";
    case kElixir:
        return "ElixirCollector.png";
    default:
        return "GoldMine.png";
    }
}
std::string ResourceBuilding::getImageForLevel(int level) const
{
    return getImageFile();
}
int ResourceBuilding::getProductionRate() const
{
    if (_level < 1 || _level > 15)
        return 0;
    return GOLD_PRODUCTION_RATES[_level];
}
int ResourceBuilding::getStorageCapacity() const
{
    if (_level < 1 || _level > 15)
        return 0;
    return STORAGE_CAPACITIES[_level];
}
int ResourceBuilding::getUpgradeCost() const
{
    if (_level < 1 || _level > 15)
        return 0;
    return UPGRADE_COSTS[_level];
}
bool ResourceBuilding::upgrade()
{
    if (!canUpgrade())
        return false;
    int cost = getUpgradeCost();
    auto& rm = ResourceManager::getInstance();
    if (!rm.hasEnough(kGold, cost))
        return false;
    if (!rm.consume(kGold, cost))
        return false;
    _level++;
    updateAppearance();
    if (_upgradeCallback)
    {
        _upgradeCallback(true, _level);
    }
    return true;
}
void ResourceBuilding::tick(float dt)
{
    if (isStorageFull())
        return;
    float productionPerSecond = static_cast<float>(getProductionRate()) / 3600.0f;
    _productionAccumulator += productionPerSecond * dt;
    if (_productionAccumulator >= 1.0f)
    {
        int produced = static_cast<int>(_productionAccumulator);
        _productionAccumulator -= static_cast<float>(produced);
        int capacity = getStorageCapacity();
        _currentStorage = std::min(_currentStorage + produced, capacity);
        if (_storageLabel)
        {
            _storageLabel->setString(std::to_string(_currentStorage));
            _storageLabel->setVisible(_currentStorage > 0);
        }
        if (isStorageFull())
        {
            showCollectHint();
        }
    }
}
int ResourceBuilding::collect()
{
    int collected = _currentStorage;
    if (collected <= 0)
        return 0;
    auto& rm = ResourceManager::getInstance();
    int actualAdded = rm.addResource(_resourceType, collected);
    _currentStorage = 0;
    _productionAccumulator = 0.0f;
    if (_storageLabel)
    {
        _storageLabel->setString("0");
        _storageLabel->setVisible(false);
    }
    hideCollectHint();
    auto scaleUp = ScaleTo::create(0.1f, this->getScale() * 1.1f);
    auto scaleDown = ScaleTo::create(0.1f, this->getScale());
    this->runAction(Sequence::create(scaleUp, scaleDown, nullptr));
    return actualAdded;
}
void ResourceBuilding::updateAppearance()
{
    auto texture = Director::getInstance()->getTextureCache()->addImage(getImageFile());
    if (texture)
    {
        this->setTexture(texture);
        this->setName(getDisplayName());
    }
}
void ResourceBuilding::showCollectHint()
{
    auto hint = this->getChildByName("collectHint");
    if (!hint)
    {
        auto hintLabel = Label::createWithSystemFont("!", "Arial", 20);
        hintLabel->setName("collectHint");
        hintLabel->setPosition(Vec2(this->getContentSize().width / 2, this->getContentSize().height + 10));
        hintLabel->setTextColor(Color4B::YELLOW);
        this->addChild(hintLabel, 100);
        auto blink = RepeatForever::create(Blink::create(1.0f, 2));
        hintLabel->runAction(blink);
    }
}
void ResourceBuilding::hideCollectHint()
{
    auto hint = this->getChildByName("collectHint");
    if (hint)
    {
        hint->removeFromParent();
    }
}