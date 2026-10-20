#include "WaterDemoController.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render3D/WaterRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"

namespace {
    // 水の色の見本(名前と色。wが透明度)
    struct WaterColor {
        const wchar_t* name;
        DirectX::XMFLOAT4 color;
    };
    constexpr WaterColor waterColors[] = {
        { L"青緑(池)",   { 0.1f, 0.4f, 0.6f, 0.55f } },
        { L"緑(藻の池)", { 0.2f, 0.6f, 0.25f, 0.6f } },
        { L"茶色(沼)",   { 0.5f, 0.3f, 0.12f, 0.75f } },
    };
    constexpr int waterColorCount = (int)(sizeof(waterColors) / sizeof(waterColors[0]));

    constexpr float waveHeightStep = 4.f;     // [1][2]を1回押すごとに変わる、波の高さ
    constexpr float maxWaveHeight = 40.f;     // 波の高さの上限
    constexpr float waveSpeedStep = 30.f;     // [3][4]を1回押すごとに変わる、波の速さ
    constexpr float maxWaveSpeed = 300.f;     // 波の速さの上限
    constexpr DirectX::XMFLOAT2 listOffset = { 40.f, -170.f };   // 左上から: 状態の一覧の1行目
    constexpr float listGap = 28.f;                              // 一覧の行の間隔
}

void WaterDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void WaterDemoController::Update(float dt) {
    if (!water) return;
    WaterSettings& settings = water->GetSettings();

    // 水面の設定を変える(次のフレームから反映される。WaterRenderer::GetSettings)
    if (Input::GetKeyDown(KEY_1)) settings.waveHeight = Math::Clamp(settings.waveHeight - waveHeightStep, 0.f, maxWaveHeight);
    if (Input::GetKeyDown(KEY_2)) settings.waveHeight = Math::Clamp(settings.waveHeight + waveHeightStep, 0.f, maxWaveHeight);
    if (Input::GetKeyDown(KEY_3)) settings.waveSpeed = Math::Clamp(settings.waveSpeed - waveSpeedStep, 0.f, maxWaveSpeed);
    if (Input::GetKeyDown(KEY_4)) settings.waveSpeed = Math::Clamp(settings.waveSpeed + waveSpeedStep, 0.f, maxWaveSpeed);
    if (Input::GetKeyDown(KEY_5)) {
        colorIndex = (colorIndex + 1) % waterColorCount;
        settings.color = waterColors[colorIndex].color;
    }
}

void WaterDemoController::Draw() {
    if (!water) return;

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

    // 水面の設定
    const WaterSettings& settings = water->GetSettings();
    swprintf_s(buf, L"[1][2] 波の高さ(waveHeight): %.0f", settings.waveHeight);
    draw(buf);
    swprintf_s(buf, L"[3][4] 波の速さ(waveSpeed): %.0f", settings.waveSpeed);
    draw(buf);
    swprintf_s(buf, L"[5] 水の色(color): %ls", waterColors[colorIndex].name);
    draw(buf);
}
