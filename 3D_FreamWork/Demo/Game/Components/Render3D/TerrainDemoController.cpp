#include "TerrainDemoController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"

namespace {
    constexpr float probeRadius = 30.f;                          // 玉の半径(地面の上に乗せる高さ)
    constexpr DirectX::XMFLOAT2 listOffset = { 40.f, -170.f };   // 左上から: 状態の一覧の1行目
    constexpr float listGap = 28.f;                              // 一覧の行の間隔
}

void TerrainDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void TerrainDemoController::Update(float dt) {
    if (!terrain) return;
    Transform& t = GetOwner()->transform;

    // 移動処理: 矢印キーで水平に動く(上=奥、右=右)。地形の範囲の外へは出ない
    if (Input::GetKeyPress(KEY_UP))    t.position.z += speed * dt;
    if (Input::GetKeyPress(KEY_DOWN))  t.position.z -= speed * dt;
    if (Input::GetKeyPress(KEY_RIGHT)) t.position.x += speed * dt;
    if (Input::GetKeyPress(KEY_LEFT))  t.position.x -= speed * dt;
    t.position.x = Math::Clamp(t.position.x, -limit, limit);
    t.position.z = Math::Clamp(t.position.z, -limit, limit);

    // 高さ: 足元の地面の高さに乗せる(地形の GetHeightAt は、見た目と同じ三角形の上で計算する)
    t.position.y = terrain->GetHeightAt(t.position.x, t.position.z) + probeRadius;
}

void TerrainDemoController::Draw() {
    if (!terrain) return;

    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;
    style.size = 20.f;
    style.color = { 1.0f, 1.0f, 1.0f, 1.0f };

    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, listOffset);
    wchar_t buf[96];
    int line = 0;
    auto draw = [&](const wchar_t* text) { Text::Draw(text, pos.x, pos.y - listGap * (float)line, style); line++; };

    // 玉の下の地面の情報(TerrainRenderer の GetHeightAt / GetNormalAt / Contains)
    const DirectX::XMFLOAT3 p = GetOwner()->transform.position;
    const DirectX::XMFLOAT3 n = terrain->GetNormalAt(p.x, p.z);
    swprintf_s(buf, L"位置: X %.0f / Z %.0f(地形の中: %ls)", p.x, p.z, terrain->Contains(p.x, p.z) ? L"はい" : L"いいえ");
    draw(buf);
    swprintf_s(buf, L"地面の高さ(GetHeightAt): %.0f", terrain->GetHeightAt(p.x, p.z));
    draw(buf);
    swprintf_s(buf, L"地面の向き(GetNormalAt): %.2f, %.2f, %.2f(Yが1に近いほど平ら)", n.x, n.y, n.z);
    draw(buf);

    // 玉の頭の上に印(3Dの位置を画面の位置に直す。z が0以下ならカメラの後ろなので出さない)
    DirectX::XMFLOAT3 screen = Image::WorldToScreen3D({ p.x, p.y + 70.f, p.z });
    if (screen.z > 0.f) {
        style.align = TextAlign::Center;
        style.color = { 1.0f, 0.9f, 0.4f, 1.0f };
        Text::Draw(L"調べる場所", screen.x, screen.y, style);
    }
}
