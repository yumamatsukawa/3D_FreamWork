#include "ShadowDemoController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Render3D/LightComponent.h"
#include "../../../../Engine/Render3D/Shadow.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"
#include <cmath>

namespace {
    constexpr float shadowDistance = 2500.f;   // カメラの前方、この大きさの範囲に影を作る
    constexpr float autoTurnSpeed = 25.f;      // 太陽が自動で回る速さ(度/秒)
    constexpr float turnSpeed = 90.f;          // 矢印キーを押している間に、太陽の向きが変わる速さ(度/秒)
    constexpr float minElevation = 10.f;       // 太陽の高さの下限(度)
    constexpr float maxElevation = 90.f;       // 太陽の高さの上限(度。真上)
    constexpr float outlineWidth = 3.f;        // 枠線の太さ(ピクセル)
    constexpr DirectX::XMFLOAT2 listOffset = { 40.f, -170.f };   // 左上から: 状態の一覧の1行目
    constexpr float listGap = 28.f;                              // 一覧の行の間隔
}

void ShadowDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");

    // 影を有効にする(平行光源 Image::GetLight() の向きで影ができる。ほかの値は既定のまま)
    Shadow::Settings& settings = Shadow::GetSettings();
    settings.enabled = true;
    settings.distance = shadowDistance;
}

void ShadowDemoController::Uninit() {
    // 設定は全シーンで共通なので、このシーンが終わったら既定(影なし)に戻す
    Shadow::SetSettings(Shadow::Settings());
}

void ShadowDemoController::ApplySettings() {
    // 太陽の向き: 水平の向き(yaw)と高さ(elevation)から、光が進む向きを決める
    const float yaw = DirectX::XMConvertToRadians(sunYaw);
    const float elevation = DirectX::XMConvertToRadians(sunElevation);
    Image::GetLight().direction = { cosf(elevation) * sinf(yaw), -sinf(elevation), cosf(elevation) * cosf(yaw) };

    // 影を落とす・受ける(Materialの castShadow / receiveShadow)
    if (targets.castSphere) targets.castSphere->GetMaterial().castShadow = sphereCasts;
    if (targets.receiveBox) targets.receiveBox->GetMaterial().receiveShadow = boxReceives;
    if (targets.spotLight) targets.spotLight->SetCastShadow(spotCasts);

    // 枠線(Materialの outlineWidth。0なら描かない)
    for (MeshRenderer* renderer : targets.outlineTargets) {
        renderer->GetMaterial().outlineColor = { 1.0f, 0.9f, 0.2f, 1.0f };
        renderer->GetMaterial().outlineWidth = outlined ? outlineWidth : 0.f;
    }
}

void ShadowDemoController::Update(float dt) {
    // 太陽の向き: 自動で回る。矢印キーを押すと、自動を止めて手で動かす(SPACEで自動に戻す)
    if (autoSun) sunYaw += autoTurnSpeed * dt;
    bool manual = false;
    if (Input::GetKeyPress(KEY_LEFT))  { sunYaw -= turnSpeed * dt; manual = true; }
    if (Input::GetKeyPress(KEY_RIGHT)) { sunYaw += turnSpeed * dt; manual = true; }
    if (Input::GetKeyPress(KEY_UP))    { sunElevation += turnSpeed * dt; manual = true; }
    if (Input::GetKeyPress(KEY_DOWN))  { sunElevation -= turnSpeed * dt; manual = true; }
    if (manual) autoSun = false;
    if (Input::GetKeyDown(KEY_SPACE)) autoSun = !autoSun;
    sunElevation = Math::Clamp(sunElevation, minElevation, maxElevation);

    // 1〜5キーで切り替え
    if (Input::GetKeyDown(KEY_1)) sphereCasts = !sphereCasts;
    if (Input::GetKeyDown(KEY_2)) boxReceives = !boxReceives;
    if (Input::GetKeyDown(KEY_3)) spotCasts = !spotCasts;
    if (Input::GetKeyDown(KEY_4)) outlined = !outlined;
    if (Input::GetKeyDown(KEY_5)) {
        Shadow::Settings& settings = Shadow::GetSettings();
        settings.pcfRadius = (settings.pcfRadius + 1) % 3;   // 影の縁のぼかし: 0=ぼかさない → 1=3x3 → 2=5x5 → 0 ...
    }

    ApplySettings();
}

void ShadowDemoController::Draw() {
    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;
    style.size = 20.f;
    style.color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 状態の一覧
    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, listOffset);
    wchar_t buf[96];
    int line = 0;
    auto draw = [&](const wchar_t* text) { Text::Draw(text, pos.x, pos.y - listGap * (float)line, style); line++; };

    swprintf_s(buf, L"太陽: %ls  向き %.0f度 / 高さ %.0f度", autoSun ? L"自動で回転(SPACEで止める)" : L"手で操作(SPACEで自動)", sunYaw, sunElevation);
    draw(buf);
    swprintf_s(buf, L"[1] 球が影を落とす(castShadow): %ls", sphereCasts ? L"ON" : L"OFF");
    draw(buf);
    swprintf_s(buf, L"[2] 板の下の箱が影を受ける(receiveShadow): %ls", boxReceives ? L"ON" : L"OFF");
    draw(buf);
    swprintf_s(buf, L"[3] スポットライトが影を落とす: %ls", spotCasts ? L"ON" : L"OFF");
    draw(buf);
    swprintf_s(buf, L"[4] 枠線(outline): %ls", outlined ? L"ON" : L"OFF");
    draw(buf);
    swprintf_s(buf, L"[5] 影の縁のぼかし(pcfRadius): %d", Shadow::GetSettings().pcfRadius);
    draw(buf);
}
