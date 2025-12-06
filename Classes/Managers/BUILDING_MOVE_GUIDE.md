# 建筑拖动移动功能使用指南

## 功能概述

用户现在可以左键点击已放置的建筑，通过长按拖动来改变建筑的位置。这项功能提供了游戏中动态调整城镇布局的能力。

## 使用流程

### 用户操作流程

```
1. 用户左键点击建筑
   ↓
2. 长按不放（约 200ms 后自动进入移动模式）
   ↓
3. 拖动建筑到新位置
   ↓
4. 释放鼠标
   ↓
5. 确认新位置或返回原位置
```

### 具体交互行为

#### 情况 A：快速点击（无拖动）
- **行为**：打开建筑升级 UI
- **效果**：显示升级界面供用户升级建筑

#### 情况 B：长按并拖动
- **行为**：进入建筑移动模式
- **效果**：
  - 显示半透明幽灵精灵预览新位置
  - 显示网格，标记可放置和不可放置区域
  - 实时显示颜色反馈（白色=可放置，红色=不可放置）

#### 情况 C：释放鼠标确认位置
- **可放置区域**：建筑移动到新位置，网格恢复
- **不可放置区域**：建筑返回原位置，显示提示信息

## 代码实现细节

### 核心类：BuildingManager

#### 新增方法

```cpp
// 进入建筑移动模式
void startMovingBuilding(BaseBuilding* building);

// 取消建筑移动（返回原位置）
void cancelMovingBuilding();

// 确认建筑移动（更新网格和建筑状态）
void confirmBuildingMove();

// 检查是否正在移动建筑
bool isMovingBuilding() const;

// 处理建筑拖拽时的触摸移动
void onBuildingTouchMoved(const cocos2d::Vec2& touchPos);

// 处理建筑拖拽时的触摸结束
void onBuildingTouchEnded(const cocos2d::Vec2& touchPos);

// 计算移动建筑时的位置
cocos2d::Vec2 calculateBuildingPositionForMoving(const cocos2d::Vec2& gridPos) const;
```

#### 新增回调

```cpp
using BuildingMovedCallback = std::function<void(BaseBuilding*, const cocos2d::Vec2&)>;
void setOnBuildingMoved(const BuildingMovedCallback& callback);
```

#### 新增成员变量

```cpp
bool _isMovingBuilding = false;                    // 是否正在移动建筑
BaseBuilding* _movingBuilding = nullptr;           // 当前移动的建筑
cocos2d::Vec2 _buildingOriginalGridPos;            // 建筑原始网格位置
cocos2d::Sprite* _movingGhostSprite = nullptr;     // 移动时的幽灵精灵
```

### 修改的方法

#### setupBuildingClickListener

```cpp
void BuildingManager::setupBuildingClickListener(BaseBuilding* building)
```

**改进说明**：
- 原来只支持点击时打开升级 UI
- 现在支持三种交互：
  1. 快速点击 → 打开升级 UI
  2. 长按拖动 → 进入移动模式
  3. 拖拽中 → 实时更新位置预览

**工作流程**：
1. `onTouchBegan` - 记录触摸起点
2. `onTouchMoved` - 判断是否长按拖动，如是则进入移动模式
3. `onTouchEnded` - 确认或取消移动

## 网格系统集成

### 网格占用管理

```
移动前：
  网格位置 A：被建筑占用 ?
  
进入移动模式：
  网格位置 A：释放（标记为空）
  
拖拽中：
  新网格位置 B：检查是否可用
  
确认新位置：
  网格位置 B：标记为被占用 ?
  
取消移动：
  网格位置 A：标记为被占用 ?（恢复原位）
```

### GridMap 交互

```cpp
// 清除原位置占用
_gridMap->markArea(_buildingOriginalGridPos, building->getGridSize(), false);

// 显示网格
_gridMap->showWholeGrid(true);

// 更新建筑预览
_gridMap->updateBuildingBase(newGridPos, building->getGridSize(), canPlace);

// 检查新位置是否可用
bool canPlace = _gridMap->checkArea(newGridPos, building->getGridSize());

// 标记新位置为被占用
_gridMap->markArea(newGridPos, building->getGridSize(), true);
```

## 场景应用示例

### 在 DraggableMapScene 中使用

```cpp
void DraggableMapScene::onBuildingClicked(BaseBuilding* building)
{
    // ... 现有的升级 UI 代码 ...
}

// 可选：监听建筑移动完成事件
void setupBuildingMoveCallback()
{
    _buildingManager->setOnBuildingMoved([this](BaseBuilding* building, const cocos2d::Vec2& newGridPos) {
        // 处理建筑移动完成逻辑
        CCLOG("Building %s moved to grid position (%.0f, %.0f)", 
              building->getDisplayName().c_str(), newGridPos.x, newGridPos.y);
        
        // 可以在这里保存建筑位置到数据库或进行其他操作
    });
}
```

## 用户反馈

### 提示信息

移动过程中显示的提示信息：

| 阶段 | 提示信息 |
|-----|--------|
| 进入移动模式 | "拖动调整建筑位置，松开鼠标后确认" |
| 拖拽中（可放置） | 显示白色幽灵精灵 |
| 拖拽中（不可放置） | 显示红色幽灵精灵 + 红色网格 |
| 确认移动 | "{建筑名称} 已移动到新位置" |
| 取消移动 | "无法在该位置放置建筑，已恢复原位置" |
| 手动取消 | "已取消移动" |

### 视觉反馈

1. **幽灵精灵**
   - 透明度：50% (opacity=150)
   - 颜色变化：白色（可放置）→ 红色（不可放置）
   - 实时跟随鼠标位置

2. **网格显示**
   - 显示整个网格
   - 高亮显示建筑占用区域
   - 绿色表示可放置，红色表示不可放置

3. **原建筑**
   - 移动过程中隐藏
   - 确认或取消时显示

## 限制和注意事项

### 技术限制

1. **建筑大小**
   - 支持任意网格大小的建筑
   - 移动时保持原有大小

2. **同时操作**
   - 一次只能移动一个建筑
   - 进行建造操作时不能移动已有建筑

3. **网格检查**
   - 建筑必须完全在游戏地图范围内
   - 不能与其他建筑重叠
   - 不能与自己重叠（除非是原位置）

### 用户注意事项

1. **位置冲突**
   - 如果目标位置被其他建筑占用，无法移动
   - 需要先移动占用位置的其他建筑

2. **网格对齐**
   - 建筑位置自动对齐到网格
   - 不支持任意自由位置

3. **数据保存**
   - 建筑移动后会更新网格信息
   - 需要在适当时机保存到数据库

## 测试建议

### 基础功能测试

- [ ] 快速点击建筑，是否正确打开升级 UI
- [ ] 长按并拖动建筑，是否进入移动模式
- [ ] 拖动时幽灵精灵是否实时跟随鼠标
- [ ] 释放时是否根据位置确认或取消移动

### 网格系统测试

- [ ] 移动到可放置位置，建筑是否正确放置
- [ ] 移动到不可放置位置，建筑是否返回原位
- [ ] 与其他建筑冲突时，是否正确拒绝
- [ ] 超出地图范围时，是否正确拒绝

### 性能测试

- [ ] 频繁移动建筑，是否有内存泄漏
- [ ] 大量建筑时，移动是否流畅
- [ ] 幽灵精灵是否正确清理

### 边界情况

- [ ] 在地图边缘移动建筑
- [ ] 同时多个建筑相邻时的移动
- [ ] 移动后立即升级建筑
- [ ] 建筑移动中切换地图

## 常见问题

**Q: 如何禁用某些建筑的移动功能？**
A: 在 `startMovingBuilding` 方法中添加检查：
```cpp
if (building->getType() == BuildingType::kTownHall) {
    showHint("大本营无法移动");
    return;
}
```

**Q: 如何限制建筑只能在特定区域移动？**
A: 在 `onBuildingTouchEnded` 中添加额外的区域检查。

**Q: 移动费用如何处理？**
A: 在 `confirmBuildingMove` 中添加资源消耗逻辑。

**Q: 如何记录建筑移动历史？**
A: 通过 `setOnBuildingMoved` 回调记录移动事件。

## 后续改进

### 可能的优化方向

1. **移动费用系统**
   - 添加移动建筑的费用（金币或圣水）
   - 根据建筑等级决定费用

2. **移动冷却时间**
   - 添加建筑移动的冷却时间
   - 防止频繁移动

3. **移动动画**
   - 平滑的过渡动画而非瞬间移动
   - 缓动函数（EaseInOut 等）

4. **快捷移动**
   - 支持拖放到其他建筑位置进行交换
   - 一键整理布局

5. **撤销功能**
   - 记录移动历史
   - 支持撤销最近的移动

---

**实现日期**：2024年
**功能版本**：1.0
**兼容性**：Cocos2d-x 4.0+

