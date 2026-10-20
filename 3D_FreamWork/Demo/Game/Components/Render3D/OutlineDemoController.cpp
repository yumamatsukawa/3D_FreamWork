#include "OutlineDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include <cstdio>

namespace {
    constexpr float minWidth = 1.f;         // 枠線の太さの下限(ピクセル)
    constexpr float maxWidth = 12.f;        // 枠線の太さの上限(ピクセル)
    constexpr float maxRim = 3.f;           // 縁の光の強さの上限
    constexpr float rimStep = 0.25f;        // 縁の光の強さを1回で変える量
    constexpr float listX = -610.f;         // 状態の一覧の左端
    constexpr float listY = 192.f;          // 一覧の1行目(説明の板の下)
    constexpr float listGap = 26.f;         // 行の間隔
    const DirectX::XMFLOAT4 outlineColor = { 1.0f, 0.9f, 0.2f, 1.0f };     // 枠線の色(黄色)
    const DirectX::XMFLOAT4 seeThroughColor = { 0.3f, 1.0f, 0.9f, 1.0f };  // 壁越しの枠線の色(水色)
    const DirectX::XMFLOAT3 rimColor = { 0.3f, 0.7f, 1.0f };               // 縁の光の色

    // 物の上に名前を出す(カメラの後ろは出さない)
    void DrawLabel(MeshRenderer* renderer, const wchar_t* label, const wchar_t* detail) {
        if (!renderer) return;
        GameObject* object = renderer->GetOwner();
        DirectX::XMFLOAT3 head = object->transform.position;
        head.y += object->transform.scale.y * 0.5f + 45.f;
        const DirectX::XMFLOAT3 screen = Image::WorldToScreen3D(head);
        if (screen.z <= 0.f) return;
        Text::Draw(label, screen.x, screen.y + 12.f, MakeDemoTextStyle(18.f, { 1.0f, 0.9f, 0.4f, 1.0f }, TextAlign::Center));
        Text::Draw(detail, screen.x, screen.y - 8.f, MakeDemoTextStyle(14.f, { 0.8f, 0.95f, 1.0f, 1.0f }, TextAlign::Center));
    }
}

void OutlineDemoController::Init() {
    ApplySettings();
}

void OutlineDemoController::ApplySettings() {
    // 枠線(SetOutline: 色・太さ。太さ0で消える)。壁越しは seeThrough を true にする
    if (outlined) outlined->SetOutline(outlineColor, outlineOn ? outlineWidth : 0.f);
    if (hidden) hidden->SetOutline(seeThroughColor, outlineOn ? outlineWidth : 0.f, seeThroughOn);

    // 縁の光(SetRimLight: 色・強さ。強さ0で消える)
    if (rimmed) rimmed->SetRimLight(rimColor, rimOn ? rimStrength : 0.f);
}

void OutlineDemoController::Update(float dt) {
    (void)dt;

    // 1・2・3キー: 枠線・壁越し・縁の光のオン/オフ
    if (Input::GetKeyDown(KEY_1)) outlineOn = !outlineOn;
    if (Input::GetKeyDown(KEY_2)) seeThroughOn = !seeThroughOn;
    if (Input::GetKeyDown(KEY_3)) rimOn = !rimOn;

    // 上下キー: 枠線の太さ、左右キー: 縁の光の強さ
    if (Input::GetKeyDown(KEY_UP)) outlineWidth += 1.f;
    if (Input::GetKeyDown(KEY_DOWN)) outlineWidth -= 1.f;
    outlineWidth = Math::Clamp(outlineWidth, minWidth, maxWidth);
    if (Input::GetKeyDown(KEY_RIGHT)) rimStrength += rimStep;
    if (Input::GetKeyDown(KEY_LEFT)) rimStrength -= rimStep;
    rimStrength = Math::Clamp(rimStrength, 0.f, maxRim);

    ApplySettings();
}

void OutlineDemoController::Draw() {
    // 物の上の名前
    DrawLabel(outlined, L"枠線", L"outlineWidth");
    DrawLabel(hidden, L"壁の後ろの球", L"outlineSeeThrough");
    DrawLabel(rimmed, L"縁の光", L"rimStrength");

    // 状態の一覧
    wchar_t buf[128];
    swprintf_s(buf, L"[1] 枠線(SetOutline): %ls   [上下] 太さ %.0f ピクセル", outlineOn ? L"ON" : L"OFF", outlineWidth);
    Text::Draw(buf, listX, listY, MakeDemoTextStyle(19.f));
    swprintf_s(buf, L"[2] 壁越しの枠線(seeThrough): %ls   OFFにすると壁の後ろの球の枠線は見えない", seeThroughOn ? L"ON" : L"OFF");
    Text::Draw(buf, listX, listY - listGap, MakeDemoTextStyle(19.f));
    swprintf_s(buf, L"[3] 縁の光(SetRimLight): %ls   [左右] 強さ %.2f", rimOn ? L"ON" : L"OFF", rimStrength);
    Text::Draw(buf, listX, listY - listGap * 2.f, MakeDemoTextStyle(19.f));
}
