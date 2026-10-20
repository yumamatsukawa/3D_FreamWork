#include "EaseRunnerController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include <cstdio>

namespace {
    constexpr float duration = 2.0f;       // 片道にかける時間(秒。全部で同じ。違うのはEaseだけ)

    // 2D版
    constexpr float startX = -300.f;       // 動き始める位置(左端)
    constexpr float endX = 300.f;          // 動く先の位置(右端)
    constexpr float labelX = -600.f;       // 名前の左端のX
    constexpr float valueX = 360.f;        // 進み具合の左端のX
    constexpr DirectX::XMFLOAT4 restColor = { 1.0f, 1.0f, 1.0f, 1.0f };      // 左端にいる時の色
    constexpr DirectX::XMFLOAT4 movedColor = { 1.0f, 0.6f, 0.6f, 1.0f };     // 右端にいる時の色(ColorToで変わる)

    // 3D版
    constexpr float scaleRate = 1.6f;      // 奥にいる時の大きさ(元の何倍か。ScaleToで変わる)
    constexpr float turnDegrees = 180.f;   // 奥にいる時の向き(Y軸の回転。RotateToで変わる)
    constexpr DirectX::XMFLOAT4 movedColor3D = { 1.0f, 0.95f, 0.5f, 1.0f };  // 奥にいる時の色
    constexpr DirectX::XMFLOAT4 laneColors[8] = {                            // 手前にいる時の色(レーンごと)
        { 0.9f, 0.3f, 0.3f, 1.0f }, { 0.9f, 0.6f, 0.3f, 1.0f }, { 0.8f, 0.8f, 0.3f, 1.0f }, { 0.4f, 0.8f, 0.4f, 1.0f },
        { 0.3f, 0.8f, 0.8f, 1.0f }, { 0.4f, 0.5f, 0.9f, 1.0f }, { 0.7f, 0.4f, 0.9f, 1.0f }, { 0.9f, 0.4f, 0.7f, 1.0f },
    };
}

void EaseRunnerController::Init() {
    // 同じオブジェクトに付いているComponentを探す(作る側で、このComponentより先に付けておく)
    tween = GetOwner()->GetComponent<TweenComponent>();
    sprite = GetOwner()->GetComponent<SpriteRenderer>();
    mesh = GetOwner()->GetComponent<MeshRenderer>();
    baseScale = GetOwner()->transform.scale;

    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");

    Restart();
}

void EaseRunnerController::Restart() {
    if (!tween) return;
    tween->Stop();
    progress = 0.f;
    if (mode == DemoMode::Mode3D) Restart3D();
    else Restart2D();
}

void EaseRunnerController::Restart2D() {
    // 左端・元の色に戻す
    GetOwner()->transform.position.x = startX;
    if (sprite) sprite->SetColor(restColor.x, restColor.y, restColor.z, restColor.w);

    // 位置・色・好きな値(進み具合)を、同じ時間・同じEaseで同時に動かす。SetLoop(true, true)=行って戻るのを繰り返す
    const float y = GetOwner()->transform.position.y;
    tween->MoveTo({ endX, y, 0.f }, duration, ease).SetLoop(true, true);
    tween->ColorTo(movedColor, duration, ease).SetLoop(true, true);
    tween->TweenValue(0.f, 1.f, duration, [this](float value) { progress = value; }, ease).SetLoop(true, true);
}

void EaseRunnerController::Restart3D() {
    // 手前・元の大きさ・向き・色に戻す
    Transform& t = GetOwner()->transform;
    t.position.z = startZ;
    t.scale = baseScale;
    t.rotate = { 0.f, 0.f, 0.f };
    const DirectX::XMFLOAT4 color = laneColors[row % 8];
    if (mesh) mesh->SetColor(color.x, color.y, color.z, color.w);

    // 位置・大きさ・向き・色・好きな値(進み具合)を、同じ時間・同じEaseで同時に動かす
    tween->MoveTo({ t.position.x, t.position.y, endZ }, duration, ease).SetLoop(true, true);
    tween->ScaleTo({ baseScale.x * scaleRate, baseScale.y * scaleRate, baseScale.z * scaleRate }, duration, ease).SetLoop(true, true);
    tween->RotateTo({ 0.f, turnDegrees, 0.f }, duration, ease).SetLoop(true, true);
    tween->ColorTo(movedColor3D, duration, ease).SetLoop(true, true);
    tween->TweenValue(0.f, 1.f, duration, [this](float value) { progress = value; }, ease).SetLoop(true, true);
}

void EaseRunnerController::Update(float dt) {
    // SPACEで全部をやり直す(動きの出だしを見比べやすい)
    if (Input::GetKeyDown(KEY_SPACE)) Restart();
}

void EaseRunnerController::Draw() {
    if (mode == DemoMode::Mode3D) Draw3D();
    else Draw2D();
}

void EaseRunnerController::Draw2D() {
    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)。画面座標は、カメラが原点なのでワールド座標と同じ
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;
    style.size = 24.f;

    // 名前(行の左)
    const float y = GetOwner()->transform.position.y;
    style.color = { 1.0f, 0.9f, 0.4f, 1.0f };
    Text::Draw(label, labelX, y, style);

    // 進み具合(行の右。TweenValueで受け取った値)
    wchar_t buf[32];
    swprintf_s(buf, L"%.2f", progress);
    style.color = { 0.7f, 1.0f, 0.7f, 1.0f };
    Text::Draw(buf, valueX, y, style);
}

void EaseRunnerController::Draw3D() {
    // 文字の見た目: 黒い縁取り、中央寄せ
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Center;
    style.size = 18.f;

    // 3Dの位置を画面の位置に直す(z が0以下なら、カメラの後ろなので出さない)
    const DirectX::XMFLOAT3 pos = GetOwner()->transform.position;

    // 名前(レーンの手前。隣と重ならないよう、1本おきに手前へずらす)
    DirectX::XMFLOAT3 labelPos = Image::WorldToScreen3D({ pos.x, -40.f, startZ - 80.f - (float)(row % 2) * 60.f });
    if (labelPos.z > 0.f) {
        style.color = { 1.0f, 0.9f, 0.4f, 1.0f };
        Text::Draw(label, labelPos.x, labelPos.y, style);
    }

    // 進み具合(立方体の上。TweenValueで受け取った値)
    DirectX::XMFLOAT3 valuePos = Image::WorldToScreen3D({ pos.x, pos.y + 80.f, pos.z });
    if (valuePos.z > 0.f) {
        wchar_t buf[32];
        swprintf_s(buf, L"%.2f", progress);
        style.color = { 0.7f, 1.0f, 0.7f, 1.0f };
        Text::Draw(buf, valuePos.x, valuePos.y, style);
    }
}
