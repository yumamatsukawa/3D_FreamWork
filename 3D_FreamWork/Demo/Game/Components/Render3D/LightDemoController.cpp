#include "LightDemoController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"
#include <cmath>

namespace {
    constexpr float turnSpeed = 90.f;       // 矢印キーを押している間に、太陽の向きが変わる速さ(度/秒)
    constexpr float minElevation = 5.f;     // 太陽の高さの下限(度)
    constexpr float maxElevation = 90.f;    // 太陽の高さの上限(度。真上)
    constexpr DirectX::XMFLOAT2 listOffset = { 40.f, -170.f };   // 左上から: 状態の一覧の1行目
    constexpr float listGap = 30.f;                              // 一覧の行の間隔
}

void LightDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
    ApplySun();
}

void LightDemoController::ApplySun() {
    // 光が進む向き: 水平の向き(yaw)と高さ(elevation)から決める(高いほど、真下へ向かう)
    const float yaw = DirectX::XMConvertToRadians(sunYaw);
    const float elevation = DirectX::XMConvertToRadians(sunElevation);
    Light& light = Image::GetLight();
    light.direction = { cosf(elevation) * sinf(yaw), -sinf(elevation), cosf(elevation) * cosf(yaw) };
    light.color = sunOn ? DirectX::XMFLOAT3{ 1.f, 1.f, 1.f } : DirectX::XMFLOAT3{ 0.f, 0.f, 0.f };
}

void LightDemoController::Update(float dt) {
    // 矢印キーで太陽の向きを変える(左右=水平に回す、上下=高さ)
    if (Input::GetKeyPress(KEY_LEFT))  sunYaw -= turnSpeed * dt;
    if (Input::GetKeyPress(KEY_RIGHT)) sunYaw += turnSpeed * dt;
    if (Input::GetKeyPress(KEY_UP))    sunElevation += turnSpeed * dt;
    if (Input::GetKeyPress(KEY_DOWN))  sunElevation -= turnSpeed * dt;
    sunElevation = Math::Clamp(sunElevation, minElevation, maxElevation);

    // 1・2・3キーで、平行光源・点光源・スポットライトのオン/オフ(オブジェクトごと SetActive する)
    if (Input::GetKeyDown(KEY_1)) sunOn = !sunOn;
    if (Input::GetKeyDown(KEY_2) && pointLight) pointLight->SetActive(!pointLight->GetIsActive());
    if (Input::GetKeyDown(KEY_3) && spotLight) spotLight->SetActive(!spotLight->GetIsActive());

    ApplySun();
}

void LightDemoController::Draw() {
    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;
    style.size = 22.f;
    style.color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 光の状態の一覧
    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, listOffset);
    wchar_t buf[96];
    swprintf_s(buf, L"[1] 平行光源(太陽): %ls  向き %.0f度 / 高さ %.0f度", sunOn ? L"ON" : L"OFF", sunYaw, sunElevation);
    Text::Draw(buf, pos.x, pos.y, style);
    swprintf_s(buf, L"[2] 点光源(オレンジの玉): %ls", (pointLight && pointLight->GetIsActive()) ? L"ON" : L"OFF");
    Text::Draw(buf, pos.x, pos.y - listGap, style);
    swprintf_s(buf, L"[3] スポットライト(白い玉): %ls", (spotLight && spotLight->GetIsActive()) ? L"ON" : L"OFF");
    Text::Draw(buf, pos.x, pos.y - listGap * 2.f, style);
}
