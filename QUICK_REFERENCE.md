# DraggableMapScene 快速参考

## 方法速查表

### 初始化方法
| 方法 | 功能 | 调用时机 |
|-----|------|---------|
| `setupMap()` | 初始化地图和建筑管理器 | `init()` 中 |
| `setupUI()` | 创建所有UI | `init()` 中 |
| `setupTouchListener()` | 设置触摸监听 | `init()` 中 |
| `setupMouseListener()` | 设置鼠标监听 | `init()` 中 |
| `setupNetworkCallbacks()` | 设置网络回调 | `init()` 中 |
| `initBuildingData()` | 初始化建筑列表 | `init()` 中 |

### 建筑相关方法
| 方法 | 功能 | 谁调用它 |
|-----|------|---------|
| `onBuildingPlaced()` | 建筑放置完成 | BuildingManager |
| `onBuildingClicked()` | 建筑被点击 | BuildingManager |
| `hideUpgradeUI()` | 隐藏升级UI | onTouchBegan() / onBuildingClicked() |
| `closeUpgradeUI()` | 公开方法（别名） | 外部代码 |

### 地图操作方法
| 方法 | 功能 | 触发事件 |
|-----|------|---------|
| `moveMap()` | 平移地图 | 触摸移动 |
| `zoomMap()` | 缩放地图 | 鼠标滚轮 |
| `switchMap()` | 切换地图 | 地图列表点击 |
| `updateBoundary()` | 更新边界 | 缩放 / 地图切换 |
| `ensureMapInBoundary()` | 确保在边界内 | 平移后 |

### 建造UI方法
| 方法 | 功能 | 调用时机 |
|-----|------|---------|
| `showConfirmButtons()` | 显示确认按钮 | 建筑放置完成 |
| `hideConfirmButtons()` | 隐藏确认按钮 | 确认/取消后 |
| `onConfirmBuilding()` | 确认建造 | 绿色按钮点击 |
| `onCancelBuilding()` | 取消建造 | 红色按钮点击 |

### UI交互方法
| 方法 | 功能 | 触发事件 |
|-----|------|---------|
| `toggleBuildingSelection()` | 切换建筑列表 | Build 按钮点击 |
| `onBuildingItemClicked()` | 选择建筑 | 列表项点击 |
| `toggleMapList()` | 切换地图列表 | Map 按钮点击 |
| `onMapItemClicked()` | 选择地图 | 列表项点击 |

---

## 关键回调链

### 建筑升级流程
```
BuildingManager::onTouchEnded()
    ↓
_onBuildingClicked(building)
    ↓
DraggableMapScene::onBuildingClicked(building)
    ↓
hideUpgradeUI()
    ↓
BuildingUpgradeUI::create(building)
    ↓
upgradeUI->show()
```

### 建筑建造确认流程
```
BuildingManager::onTouchEnded()  // 释放鼠标
    ↓
_isWaitingConfirm = true
    ↓
DraggableMapScene::showConfirmButtons()  // 显示 ✓ ✗
    ↓
用户点击按钮
    ↓
onConfirmBuilding() / onCancelBuilding()
    ↓
BuildingManager::confirmBuilding() / cancelBuilding()
    ↓
hideConfirmButtons()
```

---

## 常用代码片段

### 显示提示信息
```cpp
showBuildingHint("您的提示文本");
```

### 隐藏升级UI
```cpp
hideUpgradeUI();  // 统一处理所有 UI 类型
```

### 切换地图
```cpp
switchMap("map/Map2.png");  // 自动清理、保存、加载、恢复
```

### 获取触摸位置
```cpp
Vec2 touchPos = touch->getLocation();  // 世界坐标
```

### 转发给 BuildingManager
```cpp
if (_buildingManager) {
    _buildingManager->onTouchBegan(_lastTouchPos);
}
```

---

## 数据成员列表

### 场景属性
```cpp
cocos2d::Size _visibleSize;              // 可见区域大小
float _currentScale;                     // 当前缩放倍数
float _minScale, _maxScale;              // 缩放范围
```

### 地图相关
```cpp
cocos2d::Sprite* _mapSprite;             // 地图精灵
GridMap* _gridMap;                       // 网格系统
cocos2d::Rect _mapBoundary;              // 地图边界
std::string _currentMapName;             // 当前地图
std::vector<MapElement> _mapElements;    // 地图元素
```

### 管理器
```cpp
BuildingManager* _buildingManager;       // 建筑管理器
HeroManager* _heroManager;               // 英雄管理器
```

### UI状态
```cpp
cocos2d::ui::Button* _confirmButton;     // 确认按钮
cocos2d::ui::Button* _cancelButton;      // 取消按钮
Node* _currentUpgradeUI;                 // 当前升级UI
bool _isBuildingListVisible;             // 建筑列表是否显示
bool _isMapListVisible;                  // 地图列表是否显示
```

---

## 触摸处理优先级

**高 → 低：**

1. **升级UI** - 最高优先级
   ```
   if (_currentUpgradeUI && _currentUpgradeUI->isVisible()) {
       // 处理升级UI触摸或隐藏
   }
   ```

2. **建筑建造** - 次高
   ```
   if (_buildingManager->isInBuildingMode()) {
       // 拖拽建筑
   }
   ```

3. **英雄操作** - 中等
   ```
   if (!_buildingManager->isInBuildingMode()) {
       // 处理英雄逻辑
   }
   ```

4. **地图操作** - 最低
   ```
   // 地图平移（onTouchMoved）
   moveMap(delta);
   ```

---

## 常见问题

### Q: 如何添加新建筑类型？
A: 
1. 在 `initBuildingData()` 中添加 BuildingData
2. 在 `BuildingManager::createBuildingEntity()` 中处理创建
3. 建筑自动获得升级UI（无需修改场景代码）

### Q: 升级UI 多次创建怎么办？
A: `onBuildingClicked()` 开头调用 `hideUpgradeUI()` 自动清理旧的

### Q: 如何修改输入优先级？
A: 编辑 `onTouchBegan()` 中的【优先级1~4】部分

### Q: 如何监听地图切换事件？
A: `switchMap()` 中编辑，或使用事件系统（后续改进）

### Q: 如何添加新UI按钮？
A: 
1. 在 `setupUI()` 中创建按钮
2. 添加 click 监听
3. 创建 `onXxxButtonClicked()` 方法

---

## 快速调试

### 添加日志
```cpp
CCLOG("Building clicked: %s", building->getDisplayName().c_str());
CCLOG("Map switched successfully to %s", mapName.c_str());
CCLOG("Upgrade UI hidden");
```

### 查看状态
```cpp
// 在任意方法中检查
if (_currentUpgradeUI && _currentUpgradeUI->isVisible()) {
    CCLOG("Upgrade UI is showing");
}

CCLOG("Building mode: %s", _buildingManager->isInBuildingMode() ? "YES" : "NO");
```

### 调试地图
启用 Debug 层：
- 点击 "Grid Dev" 按钮显示/隐藏网格和方向按钮

---

## 推荐阅读顺序

1. 📄 本文件（快速参考）
2. 📄 `ARCHITECTURE.md`（完整架构）
3. 📄 源代码（DraggableMapScene.h / .cpp）
4. 🔍 BuildingManager（理解建筑逻辑）
5. 🔍 BaseBuilding（理解升级机制）

---

## 编码规范

本场景遵循的规范：
- ✅ 方法名：`onXxxClicked()`, `setupXxx()`, `showXxx()`, `hideXxx()`
- ✅ 成员变量：`_privateVar` 私有成员加下划线前缀
- ✅ 注释：`//` 单行，`/** */` 方法文档
- ✅ 分组：`// ==================== 【分组名称】 ====================`
- ✅ 优先级：`// 【优先级N】 描述`

---

**最后更新：** 2024年
**文档版本：** 1.0
**维护人：** Development Team
