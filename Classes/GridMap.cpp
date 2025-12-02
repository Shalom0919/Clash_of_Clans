#include "GridMap.h"
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Building.h
 * File Function:
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
USING_NS_CC;

GridMap* GridMap::create(const Size& mapSize, float tileSize)
{
    GridMap* ret = new (std::nothrow) GridMap();
    if (ret && ret->init(mapSize, tileSize)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GridMap::init(const Size& mapSize, float tileSize)
{
    if (!Node::init()) return false;
    _mapSize = mapSize;
    _tileSize = tileSize;

    _gridNode = DrawNode::create();
    this->addChild(_gridNode, 1);

    _baseNode = DrawNode::create();
    this->addChild(_baseNode, 2);

    // 初始化冲突地图
    _gridWidth = (int)round(mapSize.width / tileSize); // 实际上应该根据地图形状细算，这里简化处理
    _gridHeight = (int)round(mapSize.height / tileSize * 2); // ISO地图Y轴比较特殊，预留多一点空间防止越界

    // 初始化二维数组，全部为 false (空)
    _collisionMap.resize(_gridWidth, std::vector<bool>(_gridHeight, false));

    // 默认起始点位于地图中心上方（旧逻辑兼容）
    _startPixel = Vec2(_mapSize.width / 2.0f, _mapSize.height + 30.0f - _tileSize * 0.5f);
    _gridVisible = false;

    return true;
}

Vec2 GridMap::getPositionFromGrid(Vec2 gridPos)
{
    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    // 使用 _startPixel 作为 (0,0) 对应的格子中心的像素位置偏移
    float x = (gridPos.x - gridPos.y) * halfW + _startPixel.x;
    float y = _startPixel.y - (gridPos.x + gridPos.y) * halfH;

    return Vec2(x, y);
}

Vec2 GridMap::getGridPosition(Vec2 worldPosition)
{
    // 将世界坐标转换为当前节点的本地坐标
    Vec2 localPos = this->convertToNodeSpace(worldPosition);

    // 调试输出
    // CCLOG("World: (%.1f, %.1f) -> Local: (%.1f, %.1f)", 
    //       worldPosition.x, worldPosition.y, localPos.x, localPos.y);

    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    float dx = localPos.x - _startPixel.x;
    float dy = _startPixel.y - localPos.y;

    // ISO 坐标转换公式
    float x = (dy / halfH + dx / halfW) / 2.0f;
    float y = (dy / halfH - dx / halfW) / 2.0f;

    // 四舍五入到最近的整数
    int gridX = (int)round(x);
    int gridY = (int)round(y);

    // 确保不越界
    gridX = MAX(0, MIN(_gridWidth - 1, gridX));
    gridY = MAX(0, MIN(_gridHeight - 1, gridY));

    // 调试输出
    // CCLOG("Grid position: (%d, %d)", gridX, gridY);

    return Vec2(gridX, gridY);
}

// 在 showWholeGrid 函数中，修改网格绘制逻辑：

void GridMap::showWholeGrid(bool visible, const cocos2d::Size& currentBuildingSize)
{
    _gridVisible = visible;
    _gridNode->clear();
    if (!visible) return;

    // --- 配置参数 ---
    // 如果有当前建筑的尺寸，则使用该尺寸作为大格子的步长
    int bigGridStep = 3; // 默认 3x3
    if (currentBuildingSize.width > 0 && currentBuildingSize.height > 0) {
        bigGridStep = (int)currentBuildingSize.width; // 使用建筑宽度作为步长
    }

    // 颜色配置
    Color4F smallGridColor = Color4F(1.0f, 1.0f, 1.0f, 0.03f); // 小格子底色：极淡，几乎看不见，只作为底纹
    Color4F smallGridLineColor = Color4F(1.0f, 1.0f, 1.0f, 0.15f); // 小格子线：灰色，半透明
    Color4F bigGridLineColor = Color4F(1.0f, 1.0f, 1.0f, 0.35f); // 大格子线：亮白色，半透明

    // 基础尺寸计算
    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.79f;

    int maxX = _gridWidth;
    int maxY = _gridHeight;

    // ===========================================================
    // 画所有小网格
    // ===========================================================
    for (int x = 0; x < maxX; x++) {
        for (int y = 0; y < maxY; y++) {
            Vec2 center = getPositionFromGrid(Vec2(x, y));

            Vec2 p[4];
            p[0] = Vec2(center.x, center.y + halfH);
            p[1] = Vec2(center.x + halfW, center.y);
            p[2] = Vec2(center.x, center.y - halfH);
            p[3] = Vec2(center.x - halfW, center.y);

            // 画小格子
            _gridNode->drawSolidPoly(p, 4, smallGridColor);
            _gridNode->drawPoly(p, 4, true, smallGridLineColor);
        }
    }

    // ===========================================================
    // 画大网格 - 根据当前建筑尺寸确定步长
    // ===========================================================
    for (int x = 0; x < maxX; x += bigGridStep) {
        for (int y = 0; y < maxY; y += bigGridStep) {

            int currentW = (x + bigGridStep > maxX) ? (maxX - x) : bigGridStep;
            int currentH = (y + bigGridStep > maxY) ? (maxY - y) : bigGridStep;

            if (currentW <= 0 || currentH <= 0) continue;

            Vec2 topGridCenter = getPositionFromGrid(Vec2(x, y));
            Vec2 rightGridCenter = getPositionFromGrid(Vec2(x + currentW - 1, y));
            Vec2 bottomGridCenter = getPositionFromGrid(Vec2(x + currentW - 1, y + currentH - 1));
            Vec2 leftGridCenter = getPositionFromGrid(Vec2(x, y + currentH - 1));

            Vec2 p[4];
            p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);
            p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y);
            p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH);
            p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);

            _gridNode->drawPoly(p, 4, true, bigGridLineColor);
        }

        // 创建新的提示
        auto label = Label::createWithSystemFont(sizeText, "Arial", 16);
        label->setPosition(Vec2(100, Director::getInstance()->getVisibleSize().height - 100));
        label->setTextColor(Color4B::YELLOW);
        label->setName("sizeHint");
        _gridNode->addChild(label);
    }
}

void GridMap::updateBuildingBase(Vec2 gridPos, Size size, bool isValid)
{
    _baseNode->clear();

    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    Vec2 topGridCenter = getPositionFromGrid(gridPos);
    Vec2 rightGridCenter = getPositionFromGrid(gridPos + Vec2(size.width - 1, 0));
    Vec2 bottomGridCenter = getPositionFromGrid(gridPos + Vec2(size.width - 1, size.height - 1));
    Vec2 leftGridCenter = getPositionFromGrid(gridPos + Vec2(0, size.height - 1));

    Vec2 p[4];
    p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);
    p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y);
    p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH);
    p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);

    // 颜色配置 - 根据是否可以建造使用不同颜色
    Color4F color;
    Color4F borderColor;

    Color4F color = isValid ? Color4F(0.0f, 1.0f, 0.0f, 0.5f) : Color4F(1.0f, 0.0f, 0.0f, 0.5f);
    Color4F borderColor = isValid ? Color4F::GREEN : Color4F::RED;
    if (isValid) {
        color = Color4F(0.0f, 1.0f, 0.0f, 0.3f); // 绿色，更透明
        borderColor = Color4F(0.0f, 1.0f, 0.0f, 0.8f); // 绿色边框
    }
    else {
        color = Color4F(1.0f, 0.0f, 0.0f, 0.3f); // 红色，更透明
        borderColor = Color4F(1.0f, 0.0f, 0.0f, 0.8f); // 红色边框
    }

    _baseNode->drawSolidPoly(p, 4, color);
    _baseNode->drawPoly(p, 4, true, borderColor);

    // 在底座中心显示尺寸信息
    /*std::string sizeText = StringUtils::format("%dx%d", (int)size.width, (int)size.height);
    auto sizeLabel = Label::createWithSystemFont(sizeText, "Arial", 12);
    sizeLabel->setPosition((p[0] + p[2]) / 2.0f);

    if (isValid) {
        sizeLabel->setTextColor(Color4B::GREEN);
    }
    else {
        sizeLabel->setTextColor(Color4B::RED);
    }

    _baseNode->addChild(sizeLabel);*/
}

void GridMap::hideBuildingBase()
{
    _baseNode->clear();
}

bool GridMap::checkArea(Vec2 startGridPos, Size size)
{
    int startX = (int)startGridPos.x;
    int startY = (int)startGridPos.y;
    int w = (int)size.width;
    int h = (int)size.height;

    if (startX < 0 || startY < 0 || startX + w > _gridWidth || startY + h > _gridHeight) {
        return false;
    }

    for (int x = startX; x < startX + w; x++) {
        for (int y = startY; y < startY + h; y++) {
            if (_collisionMap[x][y]) {
                return false;
            }
        }
    }
    return true;
}

void GridMap::markArea(Vec2 startGridPos, Size size, bool occupied)
{
    int startX = (int)startGridPos.x;
    int startY = (int)startGridPos.y;
    int w = (int)size.width;
    int h = (int)size.height;

    if (startX < 0 || startY < 0 || startX + w > _gridWidth || startY + h > _gridHeight) return;

    for (int x = startX; x < startX + w; x++) {
        for (int y = startY; y < startY + h; y++) {
            _collisionMap[x][y] = occupied;
        }
    }
}

// 新增：设置起始像素并提供角对齐接口
void GridMap::setStartPixel(const Vec2& pixel)
{
    _startPixel = pixel;
    if (_gridVisible) showWholeGrid(true); // 重新绘制
}

Vec2 GridMap::getStartPixel() const
{
    return _startPixel;
}

void GridMap::setStartCorner(GridMap::Corner corner)
{
    // 计算基于 corner 的 startPixel 值
    Vec2 p;
    switch (corner) {
    case TOP_LEFT:
        p = Vec2(0 + _tileSize / 2.0f, _mapSize.height - _tileSize / 2.0f);
        break;
    case TOP_RIGHT:
        p = Vec2(_mapSize.width - _tileSize / 2.0f, _mapSize.height - _tileSize / 2.0f);
        break;
    case BOTTOM_LEFT:
        p = Vec2(0 + _tileSize / 2.0f, 0 + _tileSize / 2.0f);
        break;
    case BOTTOM_RIGHT:
        p = Vec2(_mapSize.width - _tileSize / 2.0f, 0 + _tileSize / 2.0f);
        break;
    case CENTER:
    default:
        p = Vec2(_mapSize.width / 2.0f, _mapSize.height / 2.0f);
        break;
    }

    // 由于我们的 ISO Y 轴换算期望 start 在上方中心，做一些微调
    p.y += _tileSize * 0.5f;

    setStartPixel(p);
}