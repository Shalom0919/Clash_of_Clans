# 资源收集系统集成指南

## 概述
本文档说明如何在 `DraggableMapScene` 中集成资源收集系统，使玩家能够点击资源建筑（金矿、圣水收集器）上方的资源图标进行收集。

## 已实现的组件

### 1. ResourceCollectionUI (..\Classes\UI\ResourceCollectionUI.h/cpp)
- **功能**：在建筑上方显示可收集的资源提示
- **特性**：
  - 自动使用 `Resources/icon/Gold.png` 或 `Resources/icon/Elixir.png`
  - 显示资源数量 (+200 等)
  - 上浮动画（2.5秒）
  - 可点击收集

### 2. ResourceCollectionManager (..\Classes\Managers\ResourceCollectionManager.h/cpp)
- **功能**：管理所有资源建筑的收集交互
- **职责**：
  - 注册资源建筑
  - 处理触摸事件
  - 调度收集逻辑

### 3. ResourceBuilding.cpp 更新
- **改动**：
  - 修改生产速率：每10秒产一次资源
  - 产量公式：`200 + (等级-1) × 100`
  - 初始化时自动创建 `ResourceCollectionUI`
  - 新增 `showCollectionHint()` 和 `hideCollectHint()` 方法

## 集成步骤

### Step 1: 在 DraggableMapScene 中添加成员变量

### Step 2: 初始化收集管理器

### Step 3: 建筑创建时注册

### Step 4: 处理触摸事件

## 工作流程

1. **资源产出**
   - 每个资源建筑每10秒产出一次资源
   - 资源存储在建筑的内部存储中

2. **提示显示**
   - 当存储满时，自动显示收集提示
   - 提示显示资源图标 + 数量
   - 提示上浮2.5秒后自动消失

3. **收集交互**
   - 玩家点击资源提示
   - 资源添加到 ResourceManager
   - 建筑的内部存储清空
   - 建筑播放收集动画

## 已使用的资源文件

- `Resources/icon/Gold.png` - 金币图标
- `Resources/icon/Elixir.png` - 圣水图标

如果这些文件不存在，系统会自动使用彩色矩形代替。

## 日志输出

系统会输出以下日志便于调试：

## 常见问题

### Q: 资源提示没有显示？
A: 检查以下几点：
1. 资源建筑是否已创建并正在 tick()
2. 存储是否已满（可通过 `isStorageFull()` 检查）
3. ResourceCollectionUI 是否成功创建（检查日志）

### Q: 点击提示无法收集？
A: 确保：
1. 在 DraggableMapScene 中创建了 ResourceCollectionManager
2. 建筑已通过 `registerBuilding()` 注册
3. 触摸事件正确转发到 `_collectionMgr->handleTouch()`

### Q: 产出速度不对？
A: 检查 ResourceBuilding.cpp 中的 tick() 函数：
- 产量应为 `PRODUCTION_RATES[_level] / 10`
- 确保 PRODUCTION_RATES 数组定义正确

## 测试建议

1. 创建金矿和圣水收集器各一个
2. 等待约10秒查看资源是否产出
3. 查看存储量显示是否正确
4. 存储满时应显示浮动资源提示
5. 点击提示应成功收集资源

## 资源收集基本状态机

````````markdown
建筑每10秒产出资源
        ↓
显示存储量标签
        ↓
存储满时触发 showCollectionHint()
        ↓
ResourceCollectionUI 显示浮动资源提示
        ↓
玩家点击资源图标
        ↓
调用 performCollection()
        ↓
资源添加到 ResourceManager
        ↓
建筑存储清空，隐藏提示