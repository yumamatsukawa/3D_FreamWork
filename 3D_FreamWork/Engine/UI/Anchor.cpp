#include "Anchor.h"
#include "../Render/Graphics.h"

using DirectX::XMFLOAT2;

namespace {
    // Anchorの並びは「3つずつ、上・中・下の段」「左から 左・中・右」なので、番号から段と列が出せる
    int GetColumn(Anchor anchor) { return (int)anchor % 3; }   // 0=左、1=中、2=右
    int GetRow(Anchor anchor) { return (int)anchor / 3; }      // 0=上、1=中、2=下
}

XMFLOAT2 GetAnchoredPosition(Anchor anchor, XMFLOAT2 offset) {
    // 基準の位置: 画面の半分の大きさ × (列 - 1) / (1 - 段)。中央が(0,0)、Y+=上
    float baseX = (GetColumn(anchor) - 1) * Graphics::width * 0.5f;
    float baseY = (1 - GetRow(anchor)) * Graphics::height * 0.5f;
    return { baseX + offset.x, baseY + offset.y };
}

XMFLOAT2 GetAnchoredCenter(Anchor anchor, XMFLOAT2 offset, XMFLOAT2 size) {
    // 四角の角(辺・中心)を基準の位置に合わせるので、中心は基準の位置から「大きさの半分」だけ内側にある
    XMFLOAT2 position = GetAnchoredPosition(anchor, offset);
    return {
        position.x + (1 - GetColumn(anchor)) * size.x * 0.5f,
        position.y + (GetRow(anchor) - 1) * size.y * 0.5f
    };
}
