#ifndef UNIT_H
#define UNIT_H

#include "cocos2d.h"
#include <map>
#include <string>

/**
 * 核心枚举定义
 * 使用 enum class 是为了类型安全，防止不同枚举值的整数混淆
 */

// 兵种类型：决定了单位的外观、血量、攻击力等
enum class UnitType
{
    kBarbarian, // 野蛮人
    kArcher,    // 弓箭手
    // ... 以后其他兵种加这里
};

// 动作类型：决定了单位当前应该播放什么动画
enum class UnitAction
{
    kRun,    // 跑步/移动
    kIdle,   // 待机/站立
    kAttack, // 攻击（第一套）
    kAttack2,// 攻击（第二套）
    kDeath   // 死亡
};

// 方向 (8方向)：用于决定播放哪个角度的贴图
// 2.5D 游戏中通常需要根据移动向量计算出这个方向
enum class UnitDirection
{
    kUp,        // 朝上 (90度)
    kUpRight,   // 右上 (45度)
    kRight,     // 右 (0度)
    kDownRight, // 右下 (-45度)
    kDown,      // 下 (-90度)
    kDownLeft,  // 左下 (-135度)
    kLeft,      // 左 (180度)
    kUpLeft     // 左上 (135度)
};

/**
 * Unit 类
 * 继承自 cocos2d::Node，意味着它可以被添加到场景中，有坐标、缩放等属性。
 */
class Unit : public cocos2d::Node
{
public:
    // ---------------------------------------------------------
    // 1. 生命周期管理 (Cocos2d-x 标准写法)
    // ---------------------------------------------------------

    // 静态工厂方法：创建并初始化 Unit 对象，自动管理内存 (autorelease)
    // 外部调用：auto myUnit = Unit::create(UnitType::kBarbarian);
    static Unit* create(UnitType type);

    // 初始化函数：执行具体的资源加载和属性设置
    virtual bool init(UnitType type);

    // 析构函数：对象销毁时调用，负责清理内存 (如释放动画资源)
    virtual ~Unit();

    // ---------------------------------------------------------
    // 2. 核心逻辑循环
    // ---------------------------------------------------------

    // 帧更新函数：Cocos2d-x 引擎每帧会自动调用它 (需要在 init 中 scheduleUpdate)
    // 所有的位移逻辑、状态检查都在这里每一帧执行一小步
    virtual void update(float dt) override;

    // ---------------------------------------------------------
    // 3. 行为控制接口 (供外部调用)
    // ---------------------------------------------------------

    // 核心指令：命令单位移动到目标位置 (像素坐标)
    // 内部会自动计算方向、播放跑步动画
    void MoveTo(const cocos2d::Vec2& target_pos);

    // 播放攻击动画 (可选择第一套或第二套攻击动画)
    void Attack(bool useSecondAttack = false);

    // 播放死亡动画并标记单位为已死亡
    void Die();

    // 检查单位是否已死亡
    bool IsDead() const { return is_dead_; }

    // 播放动画：底层接口
    // action: 做什么 (跑/站/打/死), dir: 朝哪边
    void PlayAnimation(UnitAction action, UnitDirection dir);

private:
    // ---------------------------------------------------------
    // 4. 内部数据成员
    // ---------------------------------------------------------

    // 显示组件：屏幕上真正看到的那个“人”的图片
    cocos2d::Sprite* sprite_ = nullptr;

    // 动画缓存池：防止每次播放都重新创建动画对象，提高性能
    // Key: 字符串 (如 "run_right"), Value: 动画对象指针
    std::map<std::string, cocos2d::Animation*> anim_cache_;

    // ---------------------------------------------------------
    // 5. 移动系统属性
    // ---------------------------------------------------------
    bool          is_moving_ = false;                   // 状态标记：当前是否正在移动中
    bool          is_dead_   = false;                   // 死亡标记：单位是否已死亡
    cocos2d::Vec2 target_pos_;                          // 终点坐标：我们要去哪
    cocos2d::Vec2 move_velocity_;                       // 速度向量：包含方向和速度 (dx, dy)
    float         move_speed_  = 100.0f;                // 速度数值：跑多快 (像素/秒)
    UnitDirection current_dir_ = UnitDirection::kRight; // 记录当前朝向，停止移动时保持这个朝向

    // ---------------------------------------------------------
    // 6. 内部辅助函数
    // ---------------------------------------------------------

    // 加载配置：根据 UnitType 加载对应的 plist 和 png
    void LoadConfig(UnitType type);

    // 辅助工具：读取序列帧并存入 anim_cache_
    // key: 缓存用的名字 (如 "run_up")
    // start/end: 图片序号 (如 barbarian1.png 到 barbarian8.png)
    void AddAnim(const std::string& unitName, const std::string& key, int start, int end, float delay);

    // 数学计算：根据向量差值 (目标点 - 当前点) 算出 8 个方向之一
    UnitDirection CalculateDirection(const cocos2d::Vec2& diff);
};

#endif