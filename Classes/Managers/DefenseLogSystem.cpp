#include "DefenseLogSystem.h"
#include "AccountManager.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <sstream>

USING_NS_CC;
using namespace ui;

// ==================== DefenseLog 序列化 ====================

std::string DefenseLog::serialize() const
{
    std::ostringstream oss;
    oss << attackerId << "|" << attackerName << "|" << starsLost << "|" 
        << goldLost << "|" << elixirLost << "|" << trophyChange << "|" 
        << timestamp << "|" << (isViewed ? "1" : "0");
    return oss.str();
}

DefenseLog DefenseLog::deserialize(const std::string& data)
{
    DefenseLog log;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, log.attackerId, '|');
    std::getline(iss, log.attackerName, '|');
    std::getline(iss, token, '|');
    if (!token.empty()) log.starsLost = std::stoi(token);
    std::getline(iss, token, '|');
    if (!token.empty()) log.goldLost = std::stoi(token);
    std::getline(iss, token, '|');
    if (!token.empty()) log.elixirLost = std::stoi(token);
    std::getline(iss, token, '|');
    if (!token.empty()) log.trophyChange = std::stoi(token);
    std::getline(iss, log.timestamp, '|');
    std::getline(iss, token, '|');
    log.isViewed = (token == "1");
    
    return log;
}

// ==================== DefenseLogSystem ====================

DefenseLogSystem& DefenseLogSystem::getInstance()
{
    static DefenseLogSystem instance;
    return instance;
}

void DefenseLogSystem::addDefenseLog(const DefenseLog& log)
{
    _logs.insert(_logs.begin(), log);  // 插入到最前面（最新）
    
    // 限制日志数量
    if (_logs.size() > MAX_LOGS)
    {
        _logs.resize(MAX_LOGS);
    }
    
    save();
    
    CCLOG("🛡️ 新增防守日志: 被 %s 攻击，失去 %d 金币，%d 圣水", 
          log.attackerName.c_str(), log.goldLost, log.elixirLost);
}

std::vector<DefenseLog> DefenseLogSystem::getUnviewedLogs() const
{
    std::vector<DefenseLog> unviewed;
    for (const auto& log : _logs)
    {
        if (!log.isViewed)
        {
            unviewed.push_back(log);
        }
    }
    return unviewed;
}

void DefenseLogSystem::markAllAsViewed()
{
    for (auto& log : _logs)
    {
        log.isViewed = true;
    }
    save();
}

void DefenseLogSystem::clearAllLogs()
{
    _logs.clear();
    save();
}

bool DefenseLogSystem::hasUnviewedLogs() const
{
    for (const auto& log : _logs)
    {
        if (!log.isViewed)
        {
            return true;
        }
    }
    return false;
}

void DefenseLogSystem::save()
{
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (!currentAccount)
    {
        return;
    }
    
    std::string key = "defense_log_" + currentAccount->userId;
    
    std::ostringstream oss;
    for (size_t i = 0; i < _logs.size(); ++i)
    {
        if (i > 0) oss << "\n";
        oss << _logs[i].serialize();
    }
    
    UserDefault::getInstance()->setStringForKey(key.c_str(), oss.str());
    UserDefault::getInstance()->flush();
}

void DefenseLogSystem::load()
{
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (!currentAccount)
    {
        return;
    }
    
    std::string key = "defense_log_" + currentAccount->userId;
    std::string data = UserDefault::getInstance()->getStringForKey(key.c_str(), "");
    
    _logs.clear();
    
    if (data.empty())
    {
        return;
    }
    
    std::istringstream iss(data);
    std::string line;
    
    while (std::getline(iss, line))
    {
        if (!line.empty())
        {
            _logs.push_back(DefenseLog::deserialize(line));
        }
    }
    
    CCLOG("📂 加载了 %zu 条防守日志", _logs.size());
}

void DefenseLogSystem::showDefenseLogUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto runningScene = Director::getInstance()->getRunningScene();
    if (!runningScene)
    {
        return;
    }
    
    // 创建弹窗层
    auto layer = Layer::create();
    
    // 半透明背景
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 180));
    layer->addChild(bgMask);
    
    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    layer->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, layer);
    
    // 容器
    auto container = ui::Layout::create();
    container->setContentSize(Size(600, 500));
    container->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    container->setBackGroundColor(Color3B(40, 40, 50));
    container->setBackGroundColorOpacity(255);
    container->setPosition(Vec2((visibleSize.width - 600) / 2, (visibleSize.height - 500) / 2));
    layer->addChild(container);
    
    // 标题
    auto title = Label::createWithSystemFont("防守日志", "Arial", 32);
    title->setPosition(Vec2(300, 460));
    title->setTextColor(Color4B::YELLOW);
    container->addChild(title);
    
    // 关闭按钮
    auto closeBtn = Button::create();
    closeBtn->setTitleText("X");
    closeBtn->setTitleFontSize(32);
    closeBtn->setPosition(Vec2(570, 460));
    closeBtn->addClickEventListener([layer](Ref*) {
        layer->removeFromParent();
    });
    container->addChild(closeBtn);
    
    // 日志列表
    auto listView = ListView::create();
    listView->setDirection(ui::ScrollView::Direction::VERTICAL);
    listView->setContentSize(Size(560, 380));
    listView->setPosition(Vec2(20, 20));
    listView->setBounceEnabled(true);
    listView->setScrollBarEnabled(true);
    listView->setItemsMargin(5.0f);
    container->addChild(listView);
    
    if (_logs.empty())
    {
        auto tip = Label::createWithSystemFont("暂无防守记录", "Arial", 24);
        tip->setPosition(Vec2(280, 200));
        tip->setTextColor(Color4B::GRAY);
        
        auto item = Layout::create();
        item->setContentSize(Size(560, 380));
        item->addChild(tip);
        listView->pushBackCustomItem(item);
    }
    else
    {
        for (const auto& log : _logs)
        {
            auto item = Layout::create();
            item->setContentSize(Size(560, 100));
            
            // 背景
            Color4B bgColor = log.isViewed ? Color4B(60, 60, 80, 255) : Color4B(80, 60, 60, 255);
            auto bg = LayerColor::create(bgColor, 560, 100);
            item->addChild(bg);
            
            // 攻击者信息
            auto attackerLabel = Label::createWithSystemFont(
                StringUtils::format("被 %s 攻击", log.attackerName.c_str()), 
                "Arial", 22);
            attackerLabel->setAnchorPoint(Vec2(0, 0.5f));
            attackerLabel->setPosition(Vec2(20, 70));
            attackerLabel->setTextColor(Color4B::WHITE);
            item->addChild(attackerLabel);
            
            // 时间
            auto timeLabel = Label::createWithSystemFont(log.timestamp, "Arial", 16);
            timeLabel->setAnchorPoint(Vec2(0, 0.5f));
            timeLabel->setPosition(Vec2(20, 45));
            timeLabel->setTextColor(Color4B::GRAY);
            item->addChild(timeLabel);
            
            // 损失信息
            auto lossLabel = Label::createWithSystemFont(
                StringUtils::format("失去: 💰 %d  ⚗️ %d  🏆 %d", 
                    log.goldLost, log.elixirLost, log.trophyChange),
                "Arial", 18);
            lossLabel->setAnchorPoint(Vec2(0, 0.5f));
            lossLabel->setPosition(Vec2(20, 20));
            lossLabel->setTextColor(Color4B::RED);
            item->addChild(lossLabel);
            
            // 星数
            std::string starsStr = "";
            for (int i = 0; i < log.starsLost; i++)
            {
                starsStr += "★";
            }
            for (int i = log.starsLost; i < 3; i++)
            {
                starsStr += "☆";
            }
            
            auto starsLabel = Label::createWithSystemFont(starsStr, "Arial", 28);
            starsLabel->setPosition(Vec2(480, 50));
            starsLabel->setTextColor(Color4B::YELLOW);
            item->addChild(starsLabel);
            
            listView->pushBackCustomItem(item);
        }
    }
    
    // 添加到场景
    runningScene->addChild(layer, 1000);
    
    // 标记所有为已查看
    markAllAsViewed();
    
    // 显示动画
    container->setScale(0.0f);
    container->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}
