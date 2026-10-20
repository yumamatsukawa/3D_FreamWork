#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

// 状態の1行(文字と色)
struct DemoStatusLine {
    std::wstring text;
    DirectX::XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };
};

// 左上の説明の板の下に、状態の板(半透明の背景+文字の行)を描く(物理のデモが共有する。Component の Draw の中で呼ぶ)
// width: 板の幅。行の数に合わせて高さが決まる
void DrawDemoStatus(const std::vector<DemoStatusLine>& lines, float width = 620.f);
