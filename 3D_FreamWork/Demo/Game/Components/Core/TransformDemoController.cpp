#include "TransformDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include <cstdio>

namespace {
    constexpr float infoX = -610.f;     // 状態の文字の左端
    constexpr float infoY = 190.f;      // 状態の文字の1行目(説明の板の下)
    constexpr float infoGap = 30.f;     // 行の間隔
    constexpr float minSize = 60.f;     // 親の大きさの下限
    constexpr float maxSize = 180.f;    // 親の大きさの上限
}

DirectX::XMFLOAT3 TransformDemoController::GetChildWorldPosition() const {
    // 2Dは画像の描画と同じ計算(GetWorldTransform)、3Dはメッシュの描画と同じ計算(ワールド行列)で求める
    if (mode == DemoMode::Mode2D) return child->transform.GetWorldTransform().position;

    DirectX::XMFLOAT3 position;
    DirectX::XMStoreFloat3(&position, DirectX::XMVector3Transform(DirectX::XMVectorZero(), child->transform.GetWorldMatrix()));
    return position;
}

void TransformDemoController::SetAttached(bool value) {
    if (value == attached) return;
    Transform& t = child->transform;

    if (value) {
        // つなぐ: 親から見た決まった位置・大きさに戻して、親を設定する
        t.position = childLocalPosition;
        t.scale = childLocalScale;
        t.SetParent(&parent->transform);
    }
    else {
        // 切る: 今のワールドの位置・大きさを覚えてから親を外し、その場に残す(親が動いても、もうついてこない)
        const DirectX::XMFLOAT3 worldPosition = GetChildWorldPosition();
        const Transform world = t.GetWorldTransform();
        t.SetParent(nullptr);
        t.position = worldPosition;
        t.scale = world.scale;
        if (mode == DemoMode::Mode2D) t.rotate.z = world.rotate.z;
    }
    attached = value;
}

void TransformDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    Transform& t = parent->transform;

    // 矢印キー: 親を動かす(2DはXY、3DはXZ)
    float moveX = 0.f, moveAxis = 0.f;
    if (Input::GetKeyPress(KEY_RIGHT)) moveX += 1.f;
    if (Input::GetKeyPress(KEY_LEFT)) moveX -= 1.f;
    if (Input::GetKeyPress(KEY_UP)) moveAxis += 1.f;
    if (Input::GetKeyPress(KEY_DOWN)) moveAxis -= 1.f;
    t.position.x = Math::Clamp(t.position.x + moveX * moveSpeed * dt, -moveLimit, moveLimit);
    if (mode == DemoMode::Mode3D) t.position.z = Math::Clamp(t.position.z + moveAxis * moveSpeed * dt, 300.f - moveLimit, 300.f + moveLimit);
    else t.position.y = Math::Clamp(t.position.y + moveAxis * moveSpeed * dt, -200.f, 200.f);

    // R: 親を回す(2DはZ軸、3DはY軸)。T: 3DはX軸でも回す
    if (Input::GetKeyPress(KEY_R)) {
        if (mode == DemoMode::Mode3D) t.rotate.y += turnSpeed * dt;
        else t.rotate.z += turnSpeed * dt;
    }
    if (mode == DemoMode::Mode3D && Input::GetKeyPress(KEY_T)) t.rotate.x += turnSpeed * dt;

    // 1・2: 親の大きさを変える(子も、親の大きさに合わせて位置と大きさが変わる)
    if (Input::GetKeyPress(KEY_1)) parentSize -= parentSize * scaleSpeed * dt;
    if (Input::GetKeyPress(KEY_2)) parentSize += parentSize * scaleSpeed * dt;
    parentSize = Math::Clamp(parentSize, minSize, maxSize);
    t.scale = { parentSize, parentSize, (mode == DemoMode::Mode3D) ? parentSize : 1.f };

    // SPACE: 親子をつなぐ / 切る
    if (Input::GetKeyDown(KEY_SPACE)) SetAttached(!attached);
}

void TransformDemoController::Draw() {
    const Transform& p = parent->transform;
    const Transform& c = child->transform;
    const DirectX::XMFLOAT3 world = GetChildWorldPosition();
    const bool is3D = (mode == DemoMode::Mode3D);

    // 親の位置・回転・大きさ
    wchar_t buf[128];
    float y = infoY;
    Text::Draw(L"親 (Transform)", infoX, y, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
    y -= infoGap;
    swprintf_s(buf, L"position (%.0f, %.0f, %.0f)", p.position.x, p.position.y, p.position.z);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f));
    y -= infoGap;
    if (is3D) swprintf_s(buf, L"rotate (%.0f, %.0f, %.0f)   scale %.0f", p.rotate.x, p.rotate.y, p.rotate.z, p.scale.x);
    else swprintf_s(buf, L"rotate (Z) %.0f   scale %.0f", p.rotate.z, p.scale.x);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f));

    // 子の位置: transform.position は「親から見た位置」(ローカル)。画面上の位置はワールド
    y -= infoGap * 1.4f;
    Text::Draw(attached ? L"子 (親につながっている)" : L"子 (親から切れている)", infoX, y,
        MakeDemoTextStyle(22.f, attached ? DirectX::XMFLOAT4{ 0.7f, 1.0f, 0.7f, 1.0f } : DirectX::XMFLOAT4{ 1.0f, 0.6f, 0.6f, 1.0f }));
    y -= infoGap;
    swprintf_s(buf, L"ローカル position (%.1f, %.1f, %.1f)", c.position.x, c.position.y, c.position.z);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f));
    y -= infoGap;
    swprintf_s(buf, L"ワールド position (%.0f, %.0f, %.0f)", world.x, world.y, world.z);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f));
}
