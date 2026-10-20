#include "DemoStatus.h"
#include "DemoTextStyle.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"

namespace {
    constexpr DirectX::XMFLOAT2 panelOffset = { 12.f, -150.f };   // 左上から: 板の左上の角(説明の板の下)
    constexpr DirectX::XMFLOAT2 textOffset = { 30.f, -172.f };    // 左上から: 1行目の文字の位置
    constexpr float lineGap = 25.f;                               // 行の間隔
    constexpr float textSize = 19.f;                              // 文字の大きさ
    constexpr float padding = 20.f;                               // 板の上下の余白の合計
}

void DrawDemoStatus(const std::vector<DemoStatusLine>& lines, float width) {
    if (lines.empty()) return;

    // 文字の後ろの半透明の板(行の数で高さが決まる。文字より先に描く)
    const DirectX::XMFLOAT2 size = { width, lineGap * (float)lines.size() + padding };
    Image::DrawRect(GetAnchoredCenter(Anchor::TopLeft, panelOffset, size), size, { 0.0f, 0.0f, 0.0f, 0.55f });

    // 1行ずつ、上から下へ
    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, textOffset);
    for (const DemoStatusLine& line : lines) {
        Text::Draw(line.text, pos.x, pos.y, MakeDemoTextStyle(textSize, line.color));
        pos.y -= lineGap;
    }
}
