#include "RobotCrowdManager.h"
#include "CrowdRobotController.h"
#include "../Common/DemoTextStyle.h"
#include "../../Objects/Stress/CrowdRobot.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/Visibility.h"
#include "../../../../Engine/Render3D/Lod.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"
#include "../../../../Engine/Render3D/RenderStats.h"
#include "../../../../Engine/Render3D/Shadow.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"
#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace {
    constexpr float shadowRange = 4500.f;            // カメラの前方、この大きさの範囲に影を作る(床全体が入る広さ)
    constexpr float outlineWidth = 2.f;              // 枠線の太さ(ピクセル)
    const DirectX::XMFLOAT4 outlineColor = { 1.0f, 0.9f, 0.4f, 1.0f };   // 枠線の色
    constexpr float spawnMargin = 100.f;             // 床の端からこれだけ内側に出す
    constexpr float spawnClearance = 80.f;           // ほかのロボットからこれだけ離れていればよい距離
    constexpr int spawnTries = 20;                   // 出す場所を探す回数
    constexpr DirectX::XMFLOAT2 panelOffset = { 12.f, -150.f };   // 左上から: 文字の後ろの板の左上の角
    constexpr DirectX::XMFLOAT2 panelSize = { 560.f, 290.f };     // 板の大きさ
    constexpr DirectX::XMFLOAT2 textOffset = { 30.f, -172.f };    // 左上から: 1行目の文字の位置
    constexpr float lineGap = 24.f;                  // 行の間隔

    // 体数を増やすキー(上キー・+)と、減らすキー(下キー・-)。テンキーの+-も使える
    bool IsMoreKeyDown() { return Input::GetKeyDown(KEY_UP) || Input::GetKeyDown(VK_OEM_PLUS) || Input::GetKeyDown(VK_ADD); }
    bool IsLessKeyDown() { return Input::GetKeyDown(KEY_DOWN) || Input::GetKeyDown(VK_OEM_MINUS) || Input::GetKeyDown(VK_SUBTRACT); }
}

void RobotCrowdManager::SpawnRobot() {
    // 出す場所: ランダムに探して、ほかのロボットから一番離れた点にする(重ならないように。離れていれば途中でやめる)
    const float limit = CrowdRobotController::arenaHalf - spawnMargin;
    DirectX::XMFLOAT2 best = { 0.f, 0.f };
    float bestGap = -1.f;   // 一番近いロボットまでの距離の2乗
    for (int i = 0; i < spawnTries; i++) {
        const DirectX::XMFLOAT2 candidate = { Math::RandomRange(-limit, limit), Math::RandomRange(-limit, limit) };
        float nearest = FLT_MAX;
        for (const GameObject* other : robots) {
            const float dx = other->transform.position.x - candidate.x;
            const float dz = other->transform.position.z - candidate.y;
            nearest = (std::min)(nearest, dx * dx + dz * dz);
        }
        if (nearest > bestGap) {
            bestGap = nearest;
            best = candidate;
        }
        if (bestGap >= spawnClearance * spawnClearance) break;
    }

    GameObject* robot = CreateCrowdRobot(*GetOwner()->GetScene(), best.x, best.y, physicsOn);
    ApplyLook(robot);
    robots.push_back(robot);
}

void RobotCrowdManager::SetCount(int count) {
    count = (std::max)(0, (std::min)(count, maxCount));

    // 足りない分は出し、多い分は後ろから消す
    while ((int)robots.size() < count) SpawnRobot();
    while ((int)robots.size() > count) {
        robots.back()->Destroy();
        robots.pop_back();
    }
}

void RobotCrowdManager::Rebuild() {
    // 剛体はあとから付け外しできないので、全員を消して同じ体数で作り直す
    const int count = (int)robots.size();
    for (GameObject* robot : robots) robot->Destroy();
    robots.clear();
    SetCount(count);
}

void RobotCrowdManager::ApplyLook(GameObject* robot) const {
    ModelRenderer* renderer = robot->GetComponent<ModelRenderer>();
    if (!renderer) return;
    renderer->GetMaterial().castShadow = shadowOn;
    renderer->SetOutline(outlineColor, outlineOn ? outlineWidth : 0.f);
    renderer->SetFrustumCulling(cullingOn);
}

void RobotCrowdManager::ApplySettings() {
    // 全体の設定(影・距離で省く)と、全員の見た目
    Shadow::Settings& settings = Shadow::GetSettings();
    settings.enabled = shadowOn;
    settings.distance = shadowRange;
    Lod::SetEnabled(lodOn);
    for (GameObject* robot : robots) ApplyLook(robot);
}

void RobotCrowdManager::MeasureFrame() {
    // 0.5秒ごとに平均する(毎フレームの数字だと速すぎて読めないため)。時間は倍率の影響を受けない値を使う
    const float frameTime = Time::GetUnscaledDeltaTime();
    fpsTimer += frameTime;
    fpsFrames++;
    worstFrame = (std::max)(worstFrame, frameTime);
    if (fpsTimer < fpsInterval) return;

    fps = (float)fpsFrames / fpsTimer;
    frameMs = fpsTimer / (float)fpsFrames * 1000.f;
    worstMs = worstFrame * 1000.f;
    fpsTimer = 0.f;
    fpsFrames = 0;
    worstFrame = 0.f;
}

void RobotCrowdManager::Init() {
    // 最初の体数を出して、設定を入れる
    ApplySettings();
    SetCount(startCount);
}

void RobotCrowdManager::Update(float dt) {
    (void)dt;
    MeasureFrame();

    // 上下キー・+-キー: 体数を増減
    if (IsMoreKeyDown()) SetCount((int)robots.size() + countStep);
    if (IsLessKeyDown()) SetCount((int)robots.size() - countStep);

    // Pキー: 物理あり/なし(全員を作り直す)
    if (Input::GetKeyDown(KEY_P)) {
        physicsOn = !physicsOn;
        Rebuild();
    }

    // 1〜4キー: 影・枠線・LOD・画面外を省く
    bool changed = false;
    if (Input::GetKeyDown(KEY_1)) { shadowOn = !shadowOn; changed = true; }
    if (Input::GetKeyDown(KEY_2)) { outlineOn = !outlineOn; changed = true; }
    if (Input::GetKeyDown(KEY_3)) { lodOn = !lodOn; changed = true; }
    if (Input::GetKeyDown(KEY_4)) { cullingOn = !cullingOn; changed = true; }
    if (changed) ApplySettings();
}

void RobotCrowdManager::Draw() {
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
    const DirectX::XMFLOAT4 gray = { 0.8f, 0.8f, 0.8f, 1.0f };

    // 体数
    const int count = (int)robots.size();
    swprintf_s(buf, L"ロボット: %d 体   (上下キー か + - で %d体ずつ)", count, countStep);
    line(buf, yellow);

    // 重さ(0.5秒ごとの平均)
    swprintf_s(buf, L"FPS %.1f   1フレーム %.1f ms(最長 %.1f ms)", fps, frameMs, worstMs);
    line(buf, green);
    if (count > 0) {
        swprintf_s(buf, L"1体あたり %.3f ms", frameMs / (float)count);
        line(buf, green);
    } else {
        line(L"1体あたり -", green);
    }

    // 切り替え
    line(physicsOn ? L"[P] 物理: あり(カプセルの剛体。ぶつかる)" : L"[P] 物理: なし(Transformだけで動かす)", white);
    swprintf_s(buf, L"[1] 影: %ls   [2] 枠線: %ls", shadowOn ? L"ON" : L"OFF", outlineOn ? L"ON" : L"OFF");
    line(buf, white);
    swprintf_s(buf, L"[3] LOD: %ls   [4] 画面外を省く: %ls", lodOn ? L"ON" : L"OFF", cullingOn ? L"ON" : L"OFF");
    line(buf, white);

    // 描画の数(RenderStats: 前のフレームの数)
    swprintf_s(buf, L"描画命令 通常 %d / 影 %d / 枠線 %d", RenderStats::GetDrawCallCount(), RenderStats::GetShadowDrawCount(), RenderStats::GetOutlineDrawCount());
    line(buf, cyan);
    swprintf_s(buf, L"合計: 描画命令 %d / 三角形 %d", RenderStats::GetDrawCallTotal(), RenderStats::GetTriangleCount());
    line(buf, cyan);
    swprintf_s(buf, L"省いた数: 距離 %d / 画面外 %d", RenderStats::GetLodCulledCount(), Visibility::GetCulledCount());
    line(buf, cyan);
    line(L"FPS上限は画面の更新回数(VSync)。1フレームは最長100ms", gray);
}

void RobotCrowdManager::Uninit() {
    // 設定は全シーンで共通なので、このシーンが終わったら既定に戻す
    Lod::SetEnabled(true);
    Shadow::SetSettings(Shadow::Settings());
}
