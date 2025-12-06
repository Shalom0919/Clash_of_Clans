# DraggableMapScene 架构说明

## 总体职责划分

### 【初始化】场景和UI的设置
负责游戏场景的初始化，包括地图、UI、管理器和事件监听器的创建
- `setupMap()` - 初始化地图、网格、建筑管理器和三个关键回调
- `setupUI()` - 创建所有UI按钮和列表
- `setupTouchListener()` - 设置触摸事件监听
- `setupMouseListener()` - 设置鼠标滚轮事件监听（地图缩放）
- `setupNetworkCallbacks()` - 设置网络回调
- `setupResourceDisplay()` - 显示资源UI
- `initBuildingData()` - 初始化可建造建筑列表
- `createBuildingSelection()` - 创建建筑选择列表UI
- `createMapList()` - 创建地图选择列表UI

### 【输入处理】触摸和鼠标事件
接收用户输入，按优先级转发给相应的处理器

**触摸优先级（从高到低）：**
1. **升级UI** - 如果正在显示升级界面，该界面优先处理
2. **建筑建造** - 建造模式下的建筑放置拖拽
3. **英雄操作** - 英雄选择和移动
4. **地图操作** - 地图平移（在 onTouchMoved 中处理）

```
onTouchBegan(Touch*, Event*)  -> 检查各项，返回是否处理
onTouchMoved(Touch*, Event*)  -> 转发或平移地图
onTouchEnded(Touch*, Event*)  -> 转发或触发确认按钮显示
onTouchCancelled()            -> 作为 onTouchEnded 处理
```

### 【建造UI】建筑建造的确认流程
用户放置建筑后的确认流程，由 BuildingManager 触发回调启动

```
用户拖拽建筑放置 → 释放触摸
          ↓
BuildingManager::onTouchEnded 触发
          ↓
DraggableMapScene::showConfirmButtons() 显示 ✓ 和 ✗ 按钮
          ↓
用户点击确认 → onConfirmBuilding() → BuildingManager::confirmBuilding()
或 用户点击取消 → onCancelBuilding() → BuildingManager::cancelBuilding()
```

### 【菜单交互】按钮点击和列表操作
处理UI按钮点击和列表交互
- `toggleBuildingSelection()` - 显示/隐藏建筑选择列表
- `onBuildingItemClicked()` - 处理建筑列表项点击（进入建造模式）
- `toggleMapList()` - 显示/隐藏地图选择列表
- `onMapButtonClicked()` - 处理 Map 按钮点击
- `onMapItemClicked()` - 处理地图列表项点击
- `onBattleButtonClicked()` - 处理 Attack 按钮点击（进入战斗场景）
- `onClanButtonClicked()` - 处理 Clan 按钮点击

### 【地图操作】缩放、平移、切换地图
处理地图的交互操作，维护地图边界
- `moveMap()` - 地图平移（在触摸移动时调用）
- `ensureMapInBoundary()` - 确保地图不超出可见范围
- `zoomMap()` - 地图缩放（鼠标滚轮触发）
- `updateBoundary()` - 更新地图边界检查范围
- `switchMap()` - 切换游戏地图（清理、保存、卸载、加载、恢复）

### 【地图元素】保存和恢复地图状态
管理地图上的元素（标记、装饰等）的状态保存和恢复
- `saveMapElementsState()` - 在切换地图前保存元素状态
- `restoreMapElementsState()` - 切换地图后恢复元素状态
- `createSampleMapElements()` - 创建示例地图元素
- `updateMapElementsPosition()` - 更新元素位置（缩放时）

### 【建筑交互】建筑放置和升级UI
用户与建筑的交互，包括放置和升级

```
用户点击建筑
      ↓
BuildingManager 的点击监听器触发
      ↓
BuildingManager::onTouchEnded 回调 → _onBuildingClicked
      ↓
DraggableMapScene::onBuildingClicked(building)
      ↓
hideUpgradeUI()        (隐藏旧UI)
      ↓
BuildingUpgradeUI::create(building)  (创建新UI)
      ↓
upgradeUI->show()      (显示UI)
```

**关键方法：**
- `onBuildingPlaced()` - 建筑放置成功回调（隐藏确认按钮）
- `onBuildingClicked()` - 建筑被左键点击（呼出升级UI）
- `hideUpgradeUI()` - 隐藏升级界面（统一处理各种UI类型）
- `closeUpgradeUI()` - 外部调用方法（作为 hideUpgradeUI 的别名）
- `cleanupUpgradeUI()` - 完全清理升级UI（场景销毁或地图切换时）

### 【辅助】提示信息和清理
- `showBuildingHint()` - 显示建筑相关提示信息（自动替换旧提示）
- `cleanupUpgradeUI()` - 清理升级UI（销毁/地图切换时）

### 【网络】服务器连接
- `connectToServer()` - 连接到游戏服务器
- `setupNetworkCallbacks()` - 设置服务器回调（登录、断开连接）

---

## 调用关系图

### 建筑建造流程
```
User Input
    ↓
Build Button Click
    ↓
toggleBuildingSelection() → Show Building List
    ↓
Select Building Item
    ↓
onBuildingItemClicked()
    ↓
BuildingManager::startPlacing(buildingData)
    ↓
User Drag & Place
    ↓
onTouchBegan/onTouchMoved/onTouchEnded
    ↓
BuildingManager::onTouchBegan/Moved/Ended
    ↓
showConfirmButtons() / hideConfirmButtons()
    ↓
onConfirmBuilding() / onCancelBuilding()
    ↓
BuildingManager::confirmBuilding() / cancelBuilding()
    ↓
BuildingManager::placeBuilding()
    ↓
onBuildingPlaced() callback → hideConfirmButtons()
```

### 建筑升级流程
```
User Left Click on Building
    ↓
BuildingManager::onTouchEnded()
    ↓
BuildingManager::_onBuildingClicked callback
    ↓
DraggableMapScene::onBuildingClicked(building)
    ↓
hideUpgradeUI() → 隐藏旧UI
    ↓
BuildingUpgradeUI::create(building) → 创建新UI
    ↓
upgradeUI->show()
    ↓
User Click Upgrade Button
    ↓
building->upgrade() → BaseBuilding::upgrade()
    ↓
Upgrade Success/Failure Callback
    ↓
showBuildingHint()
```

---

## 数据结构

### MapConfig
```cpp
struct MapConfig {
    float scale;              // 地图缩放倍数
    cocos2d::Vec2 startPixel; // 网格起始像素位置
    float tileSize;           // 网格单元大小
};
```

### MapElement
```cpp
struct MapElement {
    cocos2d::Node* node;           // 地图上的节点
    cocos2d::Vec2 localPosition;   // 保存的本地位置
};
```

---

## 关键设计原则

### 1. 职责清晰
- **Scene** 负责 UI 管理和输入转发
- **BuildingManager** 负责建筑的放置和管理逻辑
- **BaseBuilding** 负责建筑的属性和升级逻辑
- **GridMap** 负责网格计算

### 2. 回调机制
场景与管理器通过回调解耦：
```cpp
// setupMap 中设置的三个关键回调
_buildingManager->setOnBuildingPlaced(callback);    // 建筑放置完成
_buildingManager->setOnHint(callback);               // 提示信息
_buildingManager->setOnBuildingClicked(callback);    // 建筑被点击
```

### 3. 统一处理所有建筑
所有建筑类型都使用 `BuildingUpgradeUI`，无需区分
```cpp
// 旧方式：分别处理不同建筑类型
if (building->type == TownHall) { ... }
else if (building->type == Resource) { ... }

// 新方式：统一处理
BuildingUpgradeUI::create(building);  // 通用 UI
```

### 4. 输入优先级
明确的触摸优先级，避免冲突：
```
升级UI > 建筑建造 > 英雄操作 > 地图操作
```

---

## 常见操作

### 添加新建筑类型
1. 在 `initBuildingData()` 中添加建筑数据
2. 在 `BuildingManager::createBuildingEntity()` 中处理创建
3. 建筑自动获得升级UI（无需额外工作）

### 添加新UI元素
1. 在 `setupUI()` 中创建 UI
2. 添加点击事件监听
3. 事件处理方法遵循 `onXxxClicked` 命名规范

### 修改输入处理优先级
编辑 `onTouchBegan()` 中的优先级注释部分

---

## 测试建议

- 测试各个触摸优先级的切换
- 测试升级 UI 的显示和隐藏
- 测试地图切换时的状态清理
- 测试建筑建造的确认/取消流程
