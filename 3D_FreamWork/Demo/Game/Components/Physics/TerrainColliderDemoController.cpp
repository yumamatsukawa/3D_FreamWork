#include "TerrainColliderDemoController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"

namespace {
    constexpr DirectX::XMFLOAT2 listOffset = { 40.f, -170.f };   // 左上から: 状態の一覧の1行目
}

void TerrainColliderDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void TerrainColliderDemoController::Draw() {
    if (!walker || !terrain) return;

    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;
    style.size = 20.f;
    style.color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 歩く箱の今の位置と、その下の地面の高さ(TerrainRenderer::GetHeightAt。見た目と当たり判定と同じデータ)
    const DirectX::XMFLOAT3 p = walker->transform.position;
    wchar_t buf[96];
    swprintf_s(buf, L"箱の高さ: %.0f / 足元の地面の高さ: %.0f", p.y, terrain->GetHeightAt(p.x, p.z));
    const DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, listOffset);
    Text::Draw(buf, pos.x, pos.y, style);
}
