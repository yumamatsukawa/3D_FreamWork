#include "WorldTextDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/WorldTextRenderer.h"
#include "../../../../Engine/UI/Anchor.h"
#include "../../../../Engine/UI/Text.h"
#include <cstdio>

namespace {
    constexpr DirectX::XMFLOAT2 panelOffset = { 12.f, -150.f };   // 左上から: 文字の後ろの板の左上の角
    constexpr DirectX::XMFLOAT2 panelSize = { 640.f, 190.f };     // 板の大きさ
    constexpr DirectX::XMFLOAT2 textOffset = { 30.f, -172.f };    // 左上から: 1行目の文字の位置
    constexpr float lineGap = 25.f;                               // 行の間隔
    constexpr float bytesPerMegabyte = 1024.f * 1024.f;
}

void WorldTextDemoController::Init() {
    ApplyGridText();
    if (board) boardObject->transform.rotate.y = boardYaw;
}

void WorldTextDemoController::ApplyGridText() {
    // 同じ文字なら全部同じテクスチャを共有する(1枚)。違う文字なら1枚ずつ別のテクスチャになる
    for (size_t i = 0; i < gridSigns.size(); i++) {
        gridSigns[i]->SetText(sameText ? std::wstring(L"HELLO") : L"No." + std::to_wstring(i + 1));
    }
}

void WorldTextDemoController::Update(float dt) {
    // Bキー: ビルボード(いつもカメラの方を向く)のオン/オフ
    if (Input::GetKeyDown(KEY_B)) billboardOn = !billboardOn;
    if (billboardSign) billboardSign->SetBillboard(billboardOn);

    // Dキー: 傾く看板を裏から見た時に見えるか。左右キー: 傾く看板を回す
    if (Input::GetKeyDown(KEY_D)) doubleSided = !doubleSided;
    if (board) {
        board->SetDoubleSided(doubleSided);
        if (Input::GetKeyPress(KEY_LEFT)) boardYaw -= turnSpeed * dt;
        if (Input::GetKeyPress(KEY_RIGHT)) boardYaw += turnSpeed * dt;
        boardObject->transform.rotate.y = boardYaw;
    }

    // Cキー: カウントを増やす(文字が変わるたびに、新しい文字のテクスチャに切り替わる)
    if (Input::GetKeyDown(KEY_C) && counterSign) {
        count++;
        counterSign->SetText(L"Count: " + std::to_wstring(count));
    }

    // 1キー: 小さな看板60枚を、全部同じ文字 / 全部違う文字に切り替える
    if (Input::GetKeyDown(KEY_1)) {
        sameText = !sameText;
        ApplyGridText();
    }
}

void WorldTextDemoController::Draw() {
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
    const DirectX::XMFLOAT4 green = { 0.7f, 1.0f, 0.7f, 1.0f };

    // 共有の状態(看板の数・テクスチャの数・メモリ)
    swprintf_s(buf, L"[1] 小さな看板の文字: %ls", sameText ? L"全部同じ(共有する)" : L"全部違う(共有しない)");
    line(buf, yellow);
    swprintf_s(buf, L"看板の数: %d 枚   文字のテクスチャ: %d 枚", (int)allSigns.size(), WorldTextRenderer::GetSharedTextureCount());
    line(buf, green);
    swprintf_s(buf, L"テクスチャの合計: %.2f MB (GetSharedTextureBytes)", (float)WorldTextRenderer::GetSharedTextureBytes() / bytesPerMegabyte);
    line(buf, green);

    // 操作の状態
    swprintf_s(buf, L"[B] ビルボード: %ls   [D] 傾く看板の裏: %ls", billboardOn ? L"ON" : L"OFF", doubleSided ? L"見える" : L"見えない");
    line(buf, white);
    swprintf_s(buf, L"[左右] 傾く看板を回す(%.0f度)   [C] カウント %d", boardYaw, count);
    line(buf, white);
    line(L"右ドラッグ・Q・Eで視点を変えて、柱に隠れるのを見る", white);
}
