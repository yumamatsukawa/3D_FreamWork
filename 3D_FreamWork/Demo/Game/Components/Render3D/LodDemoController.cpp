#include "LodDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Render3D/Lod.h"
#include "../../../../Engine/Render3D/RenderStats.h"
#include "../../../../Engine/Render3D/Shadow.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"
#include <cstdio>

namespace {
    constexpr float minScale = 0.25f;       // 距離の倍率の下限
    constexpr float maxScale = 3.f;         // 距離の倍率の上限
    constexpr float scaleStep = 0.25f;      // 倍率を1回で変える量
    constexpr float shadowRange = 3500.f;   // カメラの前方、この大きさの範囲に影を作る
    constexpr float outlineWidth = 2.f;     // 柱の枠線の太さ(ピクセル)
    constexpr DirectX::XMFLOAT2 panelOffset = { 12.f, -150.f };   // 左上から: 文字の後ろの板の左上の角
    constexpr DirectX::XMFLOAT2 panelSize = { 600.f, 248.f };     // 板の大きさ
    constexpr DirectX::XMFLOAT2 textOffset = { 30.f, -172.f };    // 左上から: 1行目の文字の位置
    constexpr float lineGap = 25.f;         // 行の間隔
}

void LodDemoController::Init() {
    // 影を有効にする(柱が影を落とす。距離で省く shadowDistance の効果を見るため)
    Shadow::Settings& settings = Shadow::GetSettings();
    settings.enabled = true;
    settings.distance = shadowRange;
    ApplySettings();
}

void LodDemoController::Uninit() {
    // 設定は全シーンで共通なので、このシーンが終わったら既定に戻す
    Lod::SetEnabled(true);
    Lod::SetGlobalScale(1.f);
    Shadow::SetSettings(Shadow::Settings());
}

void LodDemoController::ApplySettings() {
    // 距離で省く設定の全体のスイッチと倍率
    Lod::SetEnabled(lodOn);
    Lod::SetGlobalScale(scale);

    // 柱の影と枠線(Material の castShadow / outlineWidth)
    for (MeshRenderer* pillar : pillars) {
        pillar->GetMaterial().castShadow = shadowOn;
        pillar->GetMaterial().outlineWidth = outlineOn ? outlineWidth : 0.f;
    }
}

void LodDemoController::Update(float dt) {
    (void)dt;

    // 1・2・3キー: LOD・影・枠線のオン/オフ
    if (Input::GetKeyDown(KEY_1)) lodOn = !lodOn;
    if (Input::GetKeyDown(KEY_2)) shadowOn = !shadowOn;
    if (Input::GetKeyDown(KEY_3)) outlineOn = !outlineOn;

    // 上下キー: 距離の倍率
    if (Input::GetKeyDown(KEY_UP)) scale += scaleStep;
    if (Input::GetKeyDown(KEY_DOWN)) scale -= scaleStep;
    scale = Math::Clamp(scale, minScale, maxScale);

    ApplySettings();
}

void LodDemoController::Draw() {
    // 文字の後ろの半透明の板(3Dの景色の上でも読めるように。文字より先に描く)
    Image::DrawRect(GetAnchoredCenter(Anchor::TopLeft, panelOffset, panelSize), panelSize, { 0.0f, 0.0f, 0.0f, 0.55f });

    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, textOffset);
    wchar_t buf[160];
    auto line = [&](const wchar_t* text, const DirectX::XMFLOAT4& color) {
        Text::Draw(text, pos.x, pos.y, MakeDemoTextStyle(19.f, color));
        pos.y -= lineGap;
    };
    const DirectX::XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 green = { 0.7f, 1.0f, 0.7f, 1.0f };

    // 操作の状態
    swprintf_s(buf, L"[1] LOD(距離で省く): %ls   [上下] 距離の倍率 x%.2f", lodOn ? L"ON" : L"OFF", Lod::GetGlobalScale());
    line(buf, yellow);
    swprintf_s(buf, L"[2] 柱の影: %ls   [3] 柱の枠線: %ls", shadowOn ? L"ON" : L"OFF", outlineOn ? L"ON" : L"OFF");
    line(buf, white);

    // 描画の重さ(RenderStats: 前のフレームの数)
    swprintf_s(buf, L"描画命令(通常): %d   三角形: %d", RenderStats::GetDrawCallCount(), RenderStats::GetDrawTriangleCount());
    line(buf, cyan);
    swprintf_s(buf, L"描画命令(影): %d   三角形: %d", RenderStats::GetShadowDrawCount(), RenderStats::GetShadowTriangleCount());
    line(buf, cyan);
    swprintf_s(buf, L"描画命令(枠線): %d   三角形: %d", RenderStats::GetOutlineDrawCount(), RenderStats::GetOutlineTriangleCount());
    line(buf, cyan);
    swprintf_s(buf, L"合計: 描画命令 %d / 三角形 %d", RenderStats::GetDrawCallTotal(), RenderStats::GetTriangleCount());
    line(buf, green);
    swprintf_s(buf, L"距離で省いた数(GetLodCulledCount): %d", RenderStats::GetLodCulledCount());
    line(buf, green);
    line(L"柱が遠いと省く距離: 描く3000 / 影2000 / 枠線1500", white);
    line(L"右ドラッグ・Q・Eで視点を変えると数が変わる", white);
}
