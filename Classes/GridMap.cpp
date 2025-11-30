#include "GridMap.h"

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
    Vec2 localPos = this->convertToNodeSpace(worldPosition);

    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    float dx = localPos.x - _startPixel.x;
    float dy = _startPixel.y - localPos.y;

    float x = (dy / halfH + dx / halfW) / 2.0f;
    float y = (dy / halfH - dx / halfW) / 2.0f;

    return Vec2(round(x), round(y));
}

void GridMap::showWholeGrid(bool visible)
{
    _gridVisible = visible;
    _gridNode->clear();
    if (!visible) return;

    // --- 配置参数 ---
    int bigGridStep = 3; // 每 3x3 算一个大格

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
    // 第一步：画所有小网格 (作为底色填充)
    // ===========================================================
    for (int x = 0; x < maxX; x++) {
        for (int y = 0; y < maxY; y++) {
            Vec2 center = getPositionFromGrid(Vec2(x, y));

            Vec2 p[4];
            p[0] = Vec2(center.x, center.y + halfH);
            p[1] = Vec2(center.x + halfW, center.y);
            p[2] = Vec2(center.x, center.y - halfH);
            p[3] = Vec2(center.x - halfW, center.y);

            // 使用 drawSolidPoly 填充淡色，让地面看起来有质感
            _gridNode->drawSolidPoly(p, 4, smallGridColor);
            // 画小网格的“边界线”
            _gridNode->drawPoly(p, 4, true, smallGridLineColor);
        }
    }

    // ===========================================================
    // 第二步：单独画大网格的“边界线”
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
            p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);       // 顶尖
            p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y);   // 右尖
            p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH); // 底尖
            p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);     // 左尖

            _gridNode->drawPoly(p, 4, true, bigGridLineColor);
        }
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
    p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);     // 顶端
    p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y); // 右端
    p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH); // 底端
    p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);   // 左端

    Color4F color = isValid ? Color4F(0.0f, 1.0f, 0.0f, 0.5f) : Color4F(1.0f, 0.0f, 0.0f, 0.5f);
    Color4F borderColor = isValid ? Color4F::GREEN : Color4F::RED;

    _baseNode->drawSolidPoly(p, 4, color);
    _baseNode->drawPoly(p, 4, true, borderColor);
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