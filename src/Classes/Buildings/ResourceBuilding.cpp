/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceBuilding.cpp
 * File Function: 资源生产/存储建筑实现
 * Author:        赵崇治、薛毓哲、刘相成
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ResourceBuilding.h"
#include "Managers/ResourceManager.h"
#include "UI/ResourceCollectionUI.h"
#include "Managers/BuildingCapacityManager.h"
#include "Managers/ResourceCollectionManager.h"

USING_NS_CC;

// 生产型建筑数据表
static const int PRODUCTION_PER_CYCLE[] = {0,    200,  300,  400,  500,  600,  700,  800,
                                       900,  1000, 1100, 1200, 1300, 1400, 1500, 1600};

static const int PRODUCER_CAPACITIES[] = {0,    500,   1000,  1500,  2000,  3000,  4000,  5000,
                                          7500, 10000, 15000, 20000, 30000, 50000, 75000, 100000};

// 存储型建筑数据表
static const int STORAGE_CAPACITIES[] = {0,      1500,   3000,   6000,   12000,  25000,  45000,  100000,
                                         150000, 200000, 250000, 300000, 400000, 500000, 750000, 1000000,
                                         1500000, 2000000};

// 升级费用表
static const int UPGRADE_COSTS[] = {0,     150,   300,    700,    1400,   3000,   7000,    14000,
                                    28000, 56000, 100000, 200000, 400000, 800000, 1500000, 3000000,
                                    6000000, 0};

// 生命值数据表
static const int PRODUCER_HP[] = {0, 400, 450, 500, 550, 600, 640, 680, 720, 780, 840, 900, 960, 1020, 1080, 1180};
static const int STORAGE_HP[] = {0,    600,  700,  800,  900,  1000, 1200, 1300, 1400,
                                 1600, 1800, 2100, 2400, 2700, 3000, 3400, 3800, 4200};
ResourceBuilding::~ResourceBuilding()
{
    // 析构时从管理器注销
    if (isProducer())
    {
        ResourceCollectionManager::getInstance()->unregisterBuilding(this);
    }
}

ResourceBuilding* ResourceBuilding::create(ResourceBuildingType buildingType, int level)
{
    ResourceBuilding* ret = new (std::nothrow) ResourceBuilding();
    if (ret)
    {
        ret->_buildingType = buildingType;
        
        // 根据建筑类型设置资源类型
        switch (buildingType)
        {
        case ResourceBuildingType::kGoldMine:
        case ResourceBuildingType::kGoldStorage:
            ret->_resourceType = kGold;
            break;
        case ResourceBuildingType::kElixirCollector:
        case ResourceBuildingType::kElixirStorage:
            ret->_resourceType = kElixir;
            break;
        }
        
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
        
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    
    // 创建存储量标签
    _storageLabel = Label::createWithSystemFont("0", "Arial", 12);
    _storageLabel->setPosition(Vec2(this->getContentSize().width / 2, -10));
    _storageLabel->setTextColor(Color4B::WHITE);
    _storageLabel->setVisible(false);
    this->addChild(_storageLabel, 100);
    
    // 设置生命值
    int hp = 400;
    if (isProducer())
    {
        int idx = std::min(_level, (int)(sizeof(PRODUCER_HP) / sizeof(int) - 1));
        hp = PRODUCER_HP[idx];
    }
    else if (isStorage())
    {
        int idx = std::min(_level, (int)(sizeof(STORAGE_HP) / sizeof(int) - 1));
        hp = STORAGE_HP[idx];
    }
    setMaxHitpoints(hp);
    initHealthBarUI();
    
    return true;
}

std::string ResourceBuilding::getDisplayName() const
{
    std::string typeName;
    switch (_buildingType)
    {
    case ResourceBuildingType::kGoldMine:
        typeName = "金矿";
        break;
    case ResourceBuildingType::kElixirCollector:
        typeName = "圣水收集器";
        break;
    case ResourceBuildingType::kGoldStorage:
        typeName = "金币仓库";
        break;
    case ResourceBuildingType::kElixirStorage:
        typeName = "圣水仓库";
        break;
    default:
        typeName = "资源建筑";
        break;
    }
    return typeName + " Lv." + std::to_string(_level);
}

int ResourceBuilding::getMaxLevel() const
{
    // 根据建筑类型返回对应的最大等级（与实际素材数量一致）
    switch (_buildingType)
    {
    case ResourceBuildingType::kGoldMine:
        return 15;  // 金矿：15级
    case ResourceBuildingType::kElixirCollector:
        return 15;  // 圣水收集器：15级
    case ResourceBuildingType::kGoldStorage:
        return 14;  // 金币仓库：14级
    case ResourceBuildingType::kElixirStorage:
        return 17;  // 圣水仓库：17级
    default:
        return 14;
    }
}

std::string ResourceBuilding::getImageFile() const
{
    return getImageForLevel(_level);
}

std::string ResourceBuilding::getImageForLevel(int level) const
{
    // 根据建筑类型和等级返回对应的图片路径
    switch (_buildingType)
    {
    case ResourceBuildingType::kGoldMine:
        return "buildings/GoldMine/Gold_Mine" + std::to_string(level) + ".png";
    case ResourceBuildingType::kElixirCollector:
        return "buildings/ElixirCollector/Elixir_Collector" + std::to_string(level) + ".png";
    case ResourceBuildingType::kGoldStorage:
        return "buildings/GoldStorage/Gold_Storage" + std::to_string(level) + ".png";
    case ResourceBuildingType::kElixirStorage:
        return "buildings/ElixirStorage/Elixir_Storage" + std::to_string(level) + ".png";
    default:
        return "buildings/GoldMine/Gold_Mine1.png";
    }
}

bool ResourceBuilding::isProducer() const
{
    return _buildingType == ResourceBuildingType::kGoldMine || 
           _buildingType == ResourceBuildingType::kElixirCollector;
}

bool ResourceBuilding::isStorage() const
{
    return _buildingType == ResourceBuildingType::kGoldStorage || 
           _buildingType == ResourceBuildingType::kElixirStorage;
}

int ResourceBuilding::getProductionRate() const
{
    if (!isProducer()) return 0;
    // 防止数组越界
    int index = std::min(_level, (int)(sizeof(PRODUCTION_PER_CYCLE) / sizeof(int) - 1));
    return PRODUCTION_PER_CYCLE[index];
}

int ResourceBuilding::getStorageCapacity() const
{
    if (isProducer())
    {
        // 生产型建筑的内部存储容量等于单次产量
        int maxIndex = sizeof(PRODUCTION_PER_CYCLE) / sizeof(int) - 1;
        int index = std::min(_level, maxIndex);
        return PRODUCTION_PER_CYCLE[index];
    }

    if (isStorage())
    {
        int maxIndex = sizeof(STORAGE_CAPACITIES) / sizeof(int) - 1;
        int index = std::min(_level, maxIndex);
        if (index < 1) return 0;
        return STORAGE_CAPACITIES[index];
    }

    return 0;
}
int ResourceBuilding::getUpgradeCost() const
{
    int maxLevel = getMaxLevel();
    if (_level < 1 || _level >= maxLevel)
        return 0;
    
    // 确保不会越界访问
    int maxIndex = sizeof(UPGRADE_COSTS) / sizeof(UPGRADE_COSTS[0]) - 1;
    if (_level > maxIndex)
        return 0;
    
    return UPGRADE_COSTS[_level];
}

// 新增：根据等级返回升级所需时间（秒）
float ResourceBuilding::getUpgradeTime() const
{
    // 升级时间表（16级）- 单位：秒
    static const int times[] = {
        0,        // Level 0 (无效)
        15,        // Level 1 -> 2: 15秒
        30,       // Level 2 -> 3: 30秒
        60,       // Level 3 -> 4: 1分钟
        120,      // Level 4 -> 5: 2分钟
        300,      // Level 5 -> 6: 5分钟
        900,      // Level 6 -> 7: 15分钟
        1800,     // Level 7 -> 8: 30分钟
        3600,     // Level 8 -> 9: 1小时
        7200,     // Level 9 -> 10: 2小时
        10800,    // Level 10 -> 11: 3小时
        14400,    // Level 11 -> 12: 4小时
        21600,    // Level 12 -> 13: 6小时
        28800,    // Level 13 -> 14: 8小时
        36000,    // Level 14 -> 15: 10小时
        64800,    // Level 15 -> 16: 18小时
        172800    // Level 16 -> 17: 2天
    };
    
    if (_level < 1 || _level > 16)
        return 0;
    
    return times[_level];
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
    if (!isProducer()) 
        return;

    // 存储已满时停止生产
    if (isStorageFull())
    {
        auto collectionUI = getCollectionUI();
        if (collectionUI)
        {
            collectionUI->updateReadyStatus(_currentStorage);
        }
        return;
    }

    _productionAccumulator += dt;
    const float PRODUCTION_INTERVAL = 15.0f;

    if (_productionAccumulator >= PRODUCTION_INTERVAL)
    {
        _productionAccumulator -= PRODUCTION_INTERVAL;

        int productionAmount = getProductionRate();
        int capacity = getStorageCapacity();
        int prevStorage = _currentStorage;
        _currentStorage = std::min(_currentStorage + productionAmount, capacity);

        if (_currentStorage > prevStorage)
        {
            auto collectionUI = getCollectionUI();
            if (collectionUI)
            {
                collectionUI->updateReadyStatus(_currentStorage);
            }
        }

        if (isStorageFull())
        {
            _productionAccumulator = 0.0f;
        }
    }
}

int ResourceBuilding::collect()
{
    if (_currentStorage <= 0) 
        return 0;

    int collected = _currentStorage;
    _currentStorage = 0;
    _productionAccumulator = 0.0f;

    auto collectionUI = getCollectionUI();
    if (collectionUI)
    {
        collectionUI->updateReadyStatus(0);
    }

    return collected;
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
    auto collectionUI = this->getChildByName<ResourceCollectionUI*>("collectionUI");
    if (collectionUI)
    {
        collectionUI->updateReadyStatus(_currentStorage);
    }
    else
    {
        // 降级方案：显示黄色感叹号
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
}

void ResourceBuilding::hideCollectHint()
{
    auto hint = this->getChildByName("collectHint");
    if (hint)
    {
        hint->removeFromParent();
    }
}

ResourceCollectionUI* ResourceBuilding::getCollectionUI() const
{
    if (!isProducer())
        return nullptr;
    return this->getChildByName<ResourceCollectionUI*>("collectionUI");
}

void ResourceBuilding::onLevelUp()
{
    // 不调用基类 onLevelUp()，ResourceBuilding 有自己的图片路径逻辑
    
    // 更新纹理
    std::string newImageFile = getImageForLevel(_level);
    if (!newImageFile.empty())
    {
        auto textureCache = Director::getInstance()->getTextureCache();
        auto texture = textureCache->addImage(newImageFile);
        if (texture)
        {
            this->setTexture(texture);
            this->setTextureRect(Rect(0, 0, texture->getContentSize().width, 
                                            texture->getContentSize().height));
        }
    }
    
    // 更新生命值
    int hp = 400;
    if (isProducer())
    {
        static const int PRODUCER_HP_TABLE[] = {0, 400, 450, 500, 550, 600, 640, 680, 720, 780, 840, 900, 960, 1020, 1080, 1180};
        int idx = std::min(_level, (int)(sizeof(PRODUCER_HP_TABLE) / sizeof(int) - 1));
        hp = PRODUCER_HP_TABLE[idx];
    }
    else if (isStorage())
    {
        static const int STORAGE_HP_TABLE[] = {0, 600, 700, 800, 900, 1000, 1200, 1300, 1400, 1600, 1800, 2100, 2400, 2700, 3000, 3400, 3800, 4200};
        int idx = std::min(_level, (int)(sizeof(STORAGE_HP_TABLE) / sizeof(int) - 1));
        hp = STORAGE_HP_TABLE[idx];
    }
    setMaxHitpoints(hp);

    // 存储型建筑通知容量管理器
    if (isStorage())
    {
        ResourceBuilding* self = this;
        ResourceType resType = _resourceType;
        
        this->scheduleOnce([self, resType](float) {
            if (self && self->getReferenceCount() > 0 && !self->isDestroyed())
            {
                if (self->getResourceType() == resType)
                {
                    BuildingCapacityManager::getInstance().registerOrUpdateBuilding(self, true);
                }
            }
        }, 0.0f, "capacity_update");
    }
    
    this->setName(getDisplayName());
    CCLOG("[ResourceBuilding] %s upgraded to Lv.%d", getDisplayName().c_str(), _level);
}

void ResourceBuilding::initCollectionUI()
{
    if (!isProducer())
        return;
    
    if (this->getChildByName("collectionUI"))
        return;
    
    auto collectionUI = ResourceCollectionUI::create(this);
    if (collectionUI)
    {
        collectionUI->setName("collectionUI");
        this->addChild(collectionUI, 1000);
        ResourceCollectionManager::getInstance()->registerBuilding(this);
    }
}
