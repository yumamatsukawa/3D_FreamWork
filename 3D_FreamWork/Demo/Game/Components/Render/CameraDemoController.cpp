#include "CameraDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoMarker.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/CameraFollowComponent.h"
#include "../../../../Engine/Render/CameraShake.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include <cmath>
#include <cstdio>

namespace {
    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4;

    // ─── 2D版 ───
    constexpr float moveSpeed2D = 450.f;                       // 自分が動く速さ(1秒あたり)
    constexpr XMFLOAT2 moveLimit2D = { 1000.f, 600.f };        // 自分が動ける範囲(中心から)
    constexpr XMFLOAT2 boundsHalf = { 900.f, 540.f };          // カメラが映してよい範囲(中心から。自分の動ける範囲より狭い)
    constexpr XMFLOAT2 noBounds = { 100000.f, 100000.f };      // 範囲を使わない時(とても広い範囲にする)
    constexpr XMFLOAT2 deadZoneSize = { 320.f, 180.f };        // デッドゾーンの大きさ(幅, 高さ)
    constexpr XMFLOAT2 offsetValue = { 0.f, 120.f };           // 対象からのずれ(対象より上を映す)
    constexpr float shakeStrength2D = 10.f;                    // 揺れの強さ(ピクセル)
    constexpr float shakeTime = 0.35f;                         // 揺れる時間(秒)
    const float smoothTimes[3] = { 0.f, 0.15f, 0.5f };         // なめらかさ(1キーで順に切り替える)

    // ─── 3D版 ───
    constexpr float moveSpeed3D = 400.f;                       // 自分が動く速さ(1秒あたり)
    constexpr float moveLimit3D = 1500.f;                      // 自分が動ける範囲(中心から)
    constexpr float mouseSensitivity = 0.004f;                 // マウス1ピクセルあたりの回転量(ラジアン)
    constexpr float zoomSpeed = 500.f;                         // Q・Eで距離が変わる速さ(1秒あたり)
    constexpr float minDistance = 150.f;                       // 三人称で近づける限界
    constexpr float maxDistance = 1500.f;                      // 三人称で離れられる限界
    constexpr float focusHeight = 40.f;                        // 三人称: 自分の位置から、見る点までの高さ
    constexpr float eyeHeight = 30.f;                          // 一人称: 自分の位置から、目までの高さ
    constexpr float minCameraY = -30.f;                        // 三人称: カメラが地面に潜らない高さ
    const float shakeStrengths3D[3] = { 4.f, 12.f, 30.f };     // 揺れの強さ(ワールドの単位。1〜3キーで選ぶ)

    // 説明の位置
    constexpr float listX = -610.f;                            // 一覧の左端
    constexpr float listY = 192.f;                             // 一覧の1行目(説明の板の下)
    constexpr float listGap = 26.f;                            // 行の間隔

    const XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
    const XMFLOAT4 green = { 0.6f, 1.0f, 0.6f, 1.0f };
    const XMFLOAT4 gray = { 0.6f, 0.6f, 0.6f, 1.0f };
    const XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 四角の枠(画面座標。中心・大きさ・線の太さ)
    void DrawFrame(XMFLOAT2 center, XMFLOAT2 size, float thickness, XMFLOAT4 color) {
        Image::DrawRect({ center.x, center.y + size.y * 0.5f }, { size.x + thickness, thickness }, color);
        Image::DrawRect({ center.x, center.y - size.y * 0.5f }, { size.x + thickness, thickness }, color);
        Image::DrawRect({ center.x - size.x * 0.5f, center.y }, { thickness, size.y + thickness }, color);
        Image::DrawRect({ center.x + size.x * 0.5f, center.y }, { thickness, size.y + thickness }, color);
    }

    // ON / OFF の文字と色
    const wchar_t* GetOnOff(bool on) { return on ? L"ON " : L"OFF"; }
    XMFLOAT4 GetOnOffColor(bool on) { return on ? green : gray; }

    // 横(x)と奥/縦(axis)の入力を、WASDと矢印キーから集める(-1〜1)
    void ReadMoveInput(float& x, float& axis) {
        x = 0.f;
        axis = 0.f;
        if (Input::GetKeyPress(KEY_D) || Input::GetKeyPress(KEY_RIGHT)) x += 1.f;
        if (Input::GetKeyPress(KEY_A) || Input::GetKeyPress(KEY_LEFT)) x -= 1.f;
        if (Input::GetKeyPress(KEY_W) || Input::GetKeyPress(KEY_UP)) axis += 1.f;
        if (Input::GetKeyPress(KEY_S) || Input::GetKeyPress(KEY_DOWN)) axis -= 1.f;
        if (x != 0.f && axis != 0.f) {
            x *= 0.7071f;
            axis *= 0.7071f;
        }
    }
}

void CameraDemoController::SetFollow(CameraFollowComponent* followComponent) {
    follow = followComponent;
    if (!follow) return;

    // 最初の設定(なめらか・範囲あり)
    follow->SetSmoothTime(smoothTimes[smoothIndex]);
    follow->SetBounds({ -boundsHalf.x, -boundsHalf.y }, { boundsHalf.x, boundsHalf.y });
}

void CameraDemoController::Init() {
    if (mode != DemoMode::Mode3D) return;

    // 3Dカメラを自分で動かすので、2Dカメラへの自動追従を止める(Uninitで戻す)
    Image::SetCamera3DAutoSync(false);
    Image::GetCamera3D().fovY = DirectX::XMConvertToRadians(60.f);
}

void CameraDemoController::Uninit() {
    // 全体の設定を戻す(揺れを止める・カメラを元の位置と自動追従へ)
    CameraShake::Stop();
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;
    if (mode == DemoMode::Mode3D) Image::SetCamera3DAutoSync(true);
}

void CameraDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    if (mode == DemoMode::Mode3D) Update3D(dt);
    else Update2D(dt);
}

void CameraDemoController::Update2D(float dt) {
    // WASD・矢印キー: 自分を動かす(カメラの部品は、このComponentの後に付いているので、動いた後の位置を追う)
    float moveX, moveY;
    ReadMoveInput(moveX, moveY);
    Transform& t = GetOwner()->transform;
    t.position.x = Math::Clamp(t.position.x + moveX * moveSpeed2D * dt, -moveLimit2D.x, moveLimit2D.x);
    t.position.y = Math::Clamp(t.position.y + moveY * moveSpeed2D * dt, -moveLimit2D.y, moveLimit2D.y);

    // SPACE: 画面を揺らす
    if (Input::GetKeyDown(KEY_SPACE)) CameraShake::Shake(shakeStrength2D, shakeTime);

    if (!follow) return;

    // 1: なめらかさ(0 → 0.15 → 0.5 → 0 ...)
    if (Input::GetKeyDown(KEY_1)) {
        smoothIndex = (smoothIndex + 1) % 3;
        follow->SetSmoothTime(smoothTimes[smoothIndex]);
    }

    // 2: デッドゾーン(この四角の中で動いても、カメラは動かない)
    if (Input::GetKeyDown(KEY_2)) {
        deadZoneOn = !deadZoneOn;
        follow->SetDeadZone(deadZoneOn ? deadZoneSize : XMFLOAT2{ 0.f, 0.f });
    }

    // 3: 範囲(範囲の外は映さない。使わない時は、とても広い範囲にする)
    if (Input::GetKeyDown(KEY_3)) {
        boundsOn = !boundsOn;
        const XMFLOAT2 half = boundsOn ? boundsHalf : noBounds;
        follow->SetBounds({ -half.x, -half.y }, { half.x, half.y });
    }

    // 4: 対象からのずれ(対象より上を映す=自分は画面の下寄りに見える)
    if (Input::GetKeyDown(KEY_4)) {
        offsetOn = !offsetOn;
        follow->SetOffset(offsetOn ? offsetValue : XMFLOAT2{ 0.f, 0.f });
    }
}

void CameraDemoController::Update3D(float dt) {
    // V: 三人称 ⇔ 一人称(一人称の間は、自分の見た目を隠す)
    if (Input::GetKeyDown(KEY_V)) {
        firstPerson = !firstPerson;
        if (body) body->SetEnabled(!firstPerson);
    }

    // 右ドラッグ: 向きを変える(カーソルは固定しないので、Escやボタンはそのまま使える)
    if (Input::GetKeyPress(MOUSE_RIGHT)) {
        const XMFLOAT2 delta = Input::GetMouseDelta();
        yaw += delta.x * mouseSensitivity;
        pitch = Math::Clamp(pitch - delta.y * mouseSensitivity, -1.3f, 1.3f);
    }

    // Q・E: 三人称のズーム
    if (Input::GetKeyPress(KEY_Q)) distance -= zoomSpeed * dt;
    if (Input::GetKeyPress(KEY_E)) distance += zoomSpeed * dt;
    distance = Math::Clamp(distance, minDistance, maxDistance);

    // 1〜3: 揺れの強さ、SPACE: 揺らす
    for (int i = 0; i < 3; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) shakeIndex = i;
    }
    if (Input::GetKeyDown(KEY_SPACE)) CameraShake::Shake(shakeStrengths3D[shakeIndex], shakeTime);

    // WASD・矢印キー: 向いている方へ動く(カメラの向きが基準。XZ平面の上)
    float moveX, moveForward;
    ReadMoveInput(moveX, moveForward);
    const XMFLOAT3 forwardXZ = { sinf(yaw), 0.f, cosf(yaw) };
    const XMFLOAT3 rightXZ = { cosf(yaw), 0.f, -sinf(yaw) };
    Transform& t = GetOwner()->transform;
    t.position.x = Math::Clamp(t.position.x + (rightXZ.x * moveX + forwardXZ.x * moveForward) * moveSpeed3D * dt, -moveLimit3D, moveLimit3D);
    t.position.z = Math::Clamp(t.position.z + (rightXZ.z * moveX + forwardXZ.z * moveForward) * moveSpeed3D * dt, -moveLimit3D, moveLimit3D);

    // カメラを置く(向きの前向きベクトルから決める)
    const float cp = cosf(pitch);
    const XMFLOAT3 forward = { cp * sinf(yaw), sinf(pitch), cp * cosf(yaw) };
    Camera& camera = Image::GetCamera3D();
    if (firstPerson) {
        // 一人称: 自分の目の位置から、向いている方を見る
        camera.position = { t.position.x, t.position.y + eyeHeight, t.position.z };
        camera.target = { camera.position.x + forward.x, camera.position.y + forward.y, camera.position.z + forward.z };
    }
    else {
        // 三人称: 自分の少し上を見て、向きの反対へ distance 離れた所に置く(地面に潜らない)
        const XMFLOAT3 focus = { t.position.x, t.position.y + focusHeight, t.position.z };
        camera.position = { focus.x - forward.x * distance, focus.y - forward.y * distance, focus.z - forward.z * distance };
        if (camera.position.y < minCameraY) camera.position.y = minCameraY;
        camera.target = focus;
    }
}

void CameraDemoController::Draw() {
    if (mode == DemoMode::Mode3D) Draw3D();
    else Draw2D();
}

void CameraDemoController::Draw2D() {
    // 一覧
    const float smooth = smoothTimes[smoothIndex];
    wchar_t buf[128];
    swprintf_s(buf, L"[1] なめらかさ: %.2f 秒   SetSmoothTime(0 → 0.15 → 0.5)", smooth);
    Text::Draw(buf, listX, listY, MakeDemoTextStyle(19.f, yellow));
    swprintf_s(buf, L"[2] デッドゾーン: %ls   SetDeadZone(黄色い四角の中では、カメラが動かない)", GetOnOff(deadZoneOn));
    Text::Draw(buf, listX, listY - listGap, MakeDemoTextStyle(19.f, GetOnOffColor(deadZoneOn)));
    swprintf_s(buf, L"[3] 範囲: %ls   SetBounds(赤い枠の外は映さない。自分は枠の外にも出られる)", GetOnOff(boundsOn));
    Text::Draw(buf, listX, listY - listGap * 2.f, MakeDemoTextStyle(19.f, GetOnOffColor(boundsOn)));
    swprintf_s(buf, L"[4] ずらし: %ls   SetOffset(対象より上を映す)", GetOnOff(offsetOn));
    Text::Draw(buf, listX, listY - listGap * 3.f, MakeDemoTextStyle(19.f, GetOnOffColor(offsetOn)));
    Text::Draw(L"[SPACE] CameraShake::Shake(10, 0.35)   画面が揺れる(カメラの位置は変えない)", listX, listY - listGap * 4.f, MakeDemoTextStyle(19.f, white));

    // 状態: カメラの位置(2Dカメラ)と、自分の位置
    const XMFLOAT3 camera = Image::GetCamera().position;
    const XMFLOAT3 self = GetOwner()->transform.position;
    swprintf_s(buf, L"カメラ (%.0f, %.0f)   自分 (%.0f, %.0f)", camera.x, camera.y, self.x, self.y);
    Text::Draw(buf, listX, listY - listGap * 5.f - 6.f, MakeDemoTextStyle(19.f, { 0.7f, 0.9f, 1.0f, 1.0f }));

    // 範囲の枠(ワールドの位置を画面の位置に直して描く。範囲を使っている時は赤、使っていない時は薄い灰色)
    const XMFLOAT2 boundsCenter = Image::WorldToScreen2D({ 0.f, 0.f, 0.f });
    DrawFrame(boundsCenter, { boundsHalf.x * 2.f, boundsHalf.y * 2.f }, 8.f, boundsOn ? XMFLOAT4{ 1.0f, 0.3f, 0.3f, 0.9f } : XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.25f });

    // デッドゾーンの枠(画面の座標。中心は、自分がいつも映る位置=ずらした分だけ下)
    if (deadZoneOn) {
        const XMFLOAT2 center = offsetOn ? XMFLOAT2{ -offsetValue.x, -offsetValue.y } : XMFLOAT2{ 0.f, 0.f };
        DrawFrame(center, deadZoneSize, 4.f, { 1.0f, 0.9f, 0.3f, 0.9f });
    }
}

void CameraDemoController::Draw3D() {
    // 一覧
    wchar_t buf[128];
    Text::Draw(std::wstring(L"[V] 視点: ") + (firstPerson ? L"一人称(目の位置から)" : L"三人称(後ろから見る)") + L"   Q・E: 三人称のズーム   右ドラッグ: 向き",
        listX, listY, MakeDemoTextStyle(19.f, yellow));
    swprintf_s(buf, L"[1〜3] 揺れの強さ: %.0f   SPACE: CameraShake::Shake(%.0f, 0.35)   揺れはカメラから見た右・上へずれる", shakeStrengths3D[shakeIndex], shakeStrengths3D[shakeIndex]);
    Text::Draw(buf, listX, listY - listGap, MakeDemoTextStyle(19.f, white));
    Text::Draw(L"WASD・矢印: 向いている方へ動く(カメラの向きが基準)", listX, listY - listGap * 2.f, MakeDemoTextStyle(19.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
    const XMFLOAT3 camera = Image::GetCamera3D().position;
    swprintf_s(buf, L"3Dカメラ (%.0f, %.0f, %.0f)   Image::GetCamera3D() を直接動かしています", camera.x, camera.y, camera.z);
    Text::Draw(buf, listX, listY - listGap * 3.f, MakeDemoTextStyle(19.f, { 0.7f, 0.9f, 1.0f, 1.0f }));

    // 三人称の時は、自分の頭の上に名前を出す
    if (!firstPerson) {
        XMFLOAT3 head = GetOwner()->transform.position;
        head.y += 60.f;
        DrawDemoLabel(DemoMode::Mode3D, head, L"自分", 0.f);
    }
}
