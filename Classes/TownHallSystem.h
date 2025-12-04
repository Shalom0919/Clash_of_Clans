#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Managers/ResourceManager.h"
#include <string>
#include <vector>

// ==================== TownHallManager ====================
class TownHallManager {
public:
    struct TownHallLevel {
        int level;
        std::string imageFile;
        int upgradeCost;
        float upgradeTime;
        std::string description;
    };

    static TownHallManager* getInstance();

    void initialize();

    const std::vector<TownHallLevel>& getAllLevels() const { return _levels; }
    const TownHallLevel* getLevel(int level) const;
    const TownHallLevel* getNextLevel(int currentLevel) const;

    bool canUpgrade(int currentLevel) const;
    int getUpgradeCost(int currentLevel) const;
    std::string getNextLevelImage(int currentLevel) const;

private:
    TownHallManager() = default;
    std::vector<TownHallLevel> _levels;
};

// ==================== TownHallBuilding ====================
class TownHallBuilding : public cocos2d::Sprite {
public:
    static TownHallBuilding* create(int level = 1);
    virtual bool init(int level);

    int getLevel() const { return _level; }
    void setLevel(int level);

    bool canUpgrade() const;
    int getUpgradeCost() const;
    bool upgrade();

    std::string getDisplayName() const;
    std::string getUpgradeInfo() const;

private:
    int _level;
    void updateAppearance();
};

// ==================== TownHallUpgradeUI ====================
class TownHallUpgradeUI : public cocos2d::Node {
public:
    static TownHallUpgradeUI* create(TownHallBuilding* building);
    virtual bool init(TownHallBuilding* building);

    void show();
    void hide();
    bool isVisible() const;

    void setPositionNearBuilding(cocos2d::Node* building);

    using UpgradeCallback = std::function<void(bool success, int newLevel)>;
    void setUpgradeCallback(const UpgradeCallback& callback) { _upgradeCallback = callback; }

private:
    TownHallBuilding* _building;
    cocos2d::ui::Button* _upgradeButton;
    cocos2d::Label* _infoLabel;
    UpgradeCallback _upgradeCallback;

    void setupUI();
    void onUpgradeClicked(cocos2d::Ref* sender);
    void updateInfo();
};

// ==================== ResourceDisplayUI ====================
class ResourceDisplayUI : public cocos2d::Node {
public:
    static ResourceDisplayUI* create();
    virtual bool init() override;

    void updateDisplay();

    void setPositionAtTopLeft();
    void setPositionAtTopRight();
    void setCustomPosition(const cocos2d::Vec2& position);

    void showResource(ResourceType type, bool show);

private:
    struct ResourceDisplay {
        cocos2d::Label* icon;
        cocos2d::Label* amount;
        cocos2d::Node* container;
    };

    std::map<ResourceType, ResourceDisplay> _displays;

    void setupResource(ResourceType type, const std::string& icon, const cocos2d::Color4B& color);
};