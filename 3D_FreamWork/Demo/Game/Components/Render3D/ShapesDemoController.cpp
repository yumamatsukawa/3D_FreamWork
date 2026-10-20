#include "ShapesDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/Text.h"
#include <cstdio>

namespace {
    constexpr float spinSpeed = 40.f;       // 形の列が回る速さ(度/秒)
    constexpr float listX = -610.f;         // 状態の一覧の左端
    constexpr float listY = 192.f;          // 一覧の1行目(説明の板の下)
    constexpr float listGap = 26.f;         // 行の間隔
}

void ShapesDemoController::AddSample(GameObject* object, Row row, const std::wstring& label, const std::wstring& detail, float labelHeight) {
    samples.push_back({ object, row, label, detail, labelHeight });
}

void ShapesDemoController::Update(float dt) {
    // 1・2・3キー: 列ごとの表示のオン/オフ(オブジェクトごと SetActive する)
    for (int i = 0; i < 3; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) rowVisible[i] = !rowVisible[i];
    }
    if (Input::GetKeyDown(KEY_4)) spinning = !spinning;

    for (const Sample& sample : samples) {
        // 表示の切り替え(列の状態に合わせる)
        sample.object->SetActive(rowVisible[ToIndex(sample.row)]);

        // 形の列は、Y軸で回して見せる(立体の形が分かりやすい)
        if (spinning && sample.row == Row::Shapes) sample.object->transform.rotate.y += spinSpeed * dt;
    }
}

void ShapesDemoController::Draw() {
    // 見本の上の名前(カメラの後ろの物は出さない)
    for (const Sample& sample : samples) {
        if (!sample.object->GetIsActive()) continue;
        DirectX::XMFLOAT3 head = sample.object->transform.position;
        head.y += sample.labelHeight;
        const DirectX::XMFLOAT3 screen = Image::WorldToScreen3D(head);
        if (screen.z <= 0.f) continue;
        Text::Draw(sample.label, screen.x, screen.y + 12.f, MakeDemoTextStyle(17.f, { 1.0f, 0.9f, 0.4f, 1.0f }, TextAlign::Center));
        Text::Draw(sample.detail, screen.x, screen.y - 6.f, MakeDemoTextStyle(13.f, { 0.8f, 0.95f, 1.0f, 1.0f }, TextAlign::Center));
    }

    // 状態の一覧
    const wchar_t* rowNames[3] = { L"[1] 形の列(Mesh::Create○○)", L"[2] 球の列(光沢・発光・縁の光)", L"[3] 立方体の列(色・画像・半透明・光を無視・枠線)" };
    for (int i = 0; i < 3; i++) {
        wchar_t buf[128];
        swprintf_s(buf, L"%ls: %ls", rowNames[i], rowVisible[i] ? L"ON" : L"OFF");
        Text::Draw(buf, listX, listY - listGap * (float)i, MakeDemoTextStyle(19.f, rowVisible[i] ? DirectX::XMFLOAT4{ 1.f, 1.f, 1.f, 1.f } : DirectX::XMFLOAT4{ 0.6f, 0.6f, 0.6f, 1.f }));
    }
    Text::Draw(spinning ? L"[4] 形の列を回す: ON" : L"[4] 形の列を回す: OFF", listX, listY - listGap * 3.f, MakeDemoTextStyle(19.f));
}
