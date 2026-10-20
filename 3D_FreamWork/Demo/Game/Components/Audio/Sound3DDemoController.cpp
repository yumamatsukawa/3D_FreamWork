#include "Sound3DDemoController.h"
#include "../Common/DemoCameraController.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Audio/Audio.h"
#include "../../../../Engine/Audio/AudioSourceComponent.h"
#include "../../../../Engine/Render/Image.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
    const std::wstring shotPath = L"Assets/Sounds/hit.wav";   // SPACEで鳴らす音

    constexpr float listenerMargin = 40.f;   // 聞く側を、動ける範囲の縁から離す距離
    constexpr float minMaxGap = 100.f;       // 聞こえなくなる距離は、最大音量の距離よりこれだけ大きくする
    constexpr float toneVolume = 0.8f;       // 音源の音量

    // 左右の目安の棒(画面の下の真ん中)
    constexpr DirectX::XMFLOAT2 gaugeCenter = { 0.f, -300.f };
    constexpr DirectX::XMFLOAT2 gaugeSize = { 400.f, 14.f };

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 blue = { 0.5f, 0.75f, 1.f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.7f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };

    float Length(DirectX::XMFLOAT2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
}

void Sound3DDemoController::Setup(GameObject* emitterObject, GameObject* listenerObject, AudioSourceComponent* audioSource, DemoCameraController* cameraController) {
    emitter = emitterObject;
    listener = listenerObject;
    source = audioSource;
    camera = cameraController;

    // 2Dと3Dで、世界の大きさが違うので、動く範囲と距離も違う
    if (mode == DemoMode::Mode2D) {
        orbitCenter = { 0.f, -100.f };
        orbitRadius = { 480.f, 90.f };
        objectSize = 50.f;
        listenerSpeed = 350.f;
        minDistance = 100.f;
        maxDistance = 600.f;
        distanceStep = 100.f;
    }
    else {
        orbitCenter = { 0.f, 25.f };
        orbitRadius = { 520.f, 240.f };
        objectSize = 80.f;
        listenerSpeed = 450.f;
        minDistance = 150.f;
        maxDistance = 1100.f;
        distanceStep = 200.f;
    }
    emitter->transform.scale = (mode == DemoMode::Mode2D) ? DirectX::XMFLOAT3{ objectSize, objectSize, 1.f } : DirectX::XMFLOAT3{ objectSize, objectSize, objectSize };
    listener->transform.scale = emitter->transform.scale;

    // 最初の位置: 音源は右端、聞く側は真ん中
    PlaceOnPlane(emitter, { orbitCenter.x + orbitRadius.x, orbitCenter.y });
    PlaceOnPlane(listener, orbitCenter);

    // 最初から鳴らす
    source->SetVolume(toneVolume);
    source->SetDistance(minDistance, maxDistance);
    source->Play();
}

void Sound3DDemoController::PlaceOnPlane(GameObject* object, DirectX::XMFLOAT2 point) {
    object->transform.position = ConvertPlaneToWorld(mode, point, objectSize * 0.5f);
    if (mode == DemoMode::Mode2D) object->transform.position.z = -1.f;   // 2D: 手前に描く
}

DirectX::XMFLOAT2 Sound3DDemoController::GetForward() const {
    // 2D: 画面の上が正面。3D: カメラが向いている水平の向き(yaw 0 で +Z、増えるほど +X へ回る)
    if (!camera) return { 0.f, 1.f };
    return { sinf(camera->GetYaw()), cosf(camera->GetYaw()) };
}

DirectX::XMFLOAT2 Sound3DDemoController::GetRight() const {
    const DirectX::XMFLOAT2 forward = GetForward();
    return { forward.y, -forward.x };
}

void Sound3DDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused() || !source) return;

    // 1: 鳴らす/止める
    if (Input::GetKeyDown(KEY_1)) {
        if (source->IsPlaying()) source->Stop();
        else source->Play();
    }

    // 2: 位置で聞こえ方を変えるか(OFFにすると、どこにいても同じ音量で真ん中から聞こえる)
    if (Input::GetKeyDown(KEY_2)) {
        spatial = !spatial;
        source->SetSpatial(spatial);
    }

    // ↑↓: 聞こえなくなる距離(SetDistance)
    if (Input::GetKeyDown(KEY_UP)) maxDistance += distanceStep;
    if (Input::GetKeyDown(KEY_DOWN)) maxDistance -= distanceStep;
    maxDistance = Math::Clamp(maxDistance, minDistance + minMaxGap, distanceStep * 12.f);
    source->SetDistance(minDistance, maxDistance);

    // ←→: 音源が回る速さ
    if (Input::GetKeyDown(KEY_RIGHT)) orbitSpeed = (std::min)(orbitSpeed + 0.4f, maxOrbitSpeed);
    if (Input::GetKeyDown(KEY_LEFT)) orbitSpeed = (std::max)(orbitSpeed - 0.4f, 0.f);

    // SPACE: 今の音源の場所に、1回だけ鳴らす(鳴らした場所は動かない)
    if (Input::GetKeyDown(KEY_SPACE)) {
        shotPosition = emitter->transform.position;
        shotTimer = shotRingSeconds;
        Audio::PlayOneShotAt(shotPath, shotPosition);
    }
    shotTimer = (std::max)(shotTimer - dt, 0.f);

    // 音源が回る(AudioSourceComponent が、毎フレームこの物の位置を聞こえ方に反映する)
    angle += orbitSpeed * dt;
    PlaceOnPlane(emitter, { orbitCenter.x + cosf(angle) * orbitRadius.x, orbitCenter.y + sinf(angle) * orbitRadius.y });

    // WASD: 聞く側を動かす(3Dはカメラの向きを基準に、前後左右)
    const DirectX::XMFLOAT2 forward = GetForward();
    const DirectX::XMFLOAT2 right = GetRight();
    DirectX::XMFLOAT2 move = { 0.f, 0.f };
    if (Input::GetKeyPress(KEY_W)) { move.x += forward.x; move.y += forward.y; }
    if (Input::GetKeyPress(KEY_S)) { move.x -= forward.x; move.y -= forward.y; }
    if (Input::GetKeyPress(KEY_D)) { move.x += right.x; move.y += right.y; }
    if (Input::GetKeyPress(KEY_A)) { move.x -= right.x; move.y -= right.y; }
    const float moveLength = Length(move);
    if (moveLength > 0.001f) {
        DirectX::XMFLOAT2 point = ConvertWorldToPlane(mode, listener->transform.position);
        point.x += move.x / moveLength * listenerSpeed * dt;
        point.y += move.y / moveLength * listenerSpeed * dt;
        PlaceOnPlane(listener, ClampToDemoField(field, point, listenerMargin));
    }
}

void Sound3DDemoController::Draw() {
    if (!source) return;

    const DirectX::XMFLOAT3 emitterPos = emitter->transform.position;
    const DirectX::XMFLOAT3 listenerPos = listener->transform.position;

    // 音源の周りの輪: 緑=ここまでは最大音量 / 黄=ここで聞こえなくなる(位置を使う時だけ)
    if (spatial) {
        DrawDemoRing(mode, emitterPos, minDistance, green, 48);
        DrawDemoRing(mode, emitterPos, maxDistance, yellow, 96);
    }

    // 聞く側の正面(白い線)
    const DirectX::XMFLOAT2 forward = GetForward();
    const DirectX::XMFLOAT3 front = (mode == DemoMode::Mode2D)
        ? DirectX::XMFLOAT3{ listenerPos.x + forward.x * 90.f, listenerPos.y + forward.y * 90.f, listenerPos.z }
        : DirectX::XMFLOAT3{ listenerPos.x + forward.x * 140.f, listenerPos.y, listenerPos.z + forward.y * 140.f };
    DrawDemoLine(mode, listenerPos, front, white);

    // SPACEで鳴らした場所(広がる輪)
    if (shotTimer > 0.f) {
        const float t = 1.f - shotTimer / shotRingSeconds;
        DrawDemoRing(mode, shotPosition, 30.f + t * minDistance, orange, 32);
    }

    // 名前
    DrawDemoLabel(mode, emitterPos, L"Source", objectSize * 0.5f + 14.f);
    DrawDemoLabel(mode, listenerPos, L"Listener (WASD)", objectSize * 0.5f + 14.f);

    // 音源の向きと距離(X3DAudioの計算に近い目安)
    const DirectX::XMFLOAT2 emitterPoint = ConvertWorldToPlane(mode, emitterPos);
    const DirectX::XMFLOAT2 listenerPoint = ConvertWorldToPlane(mode, listenerPos);
    const DirectX::XMFLOAT2 relative = { emitterPoint.x - listenerPoint.x, emitterPoint.y - listenerPoint.y };
    const float distance = Length(relative);
    const DirectX::XMFLOAT2 right = GetRight();
    float pan = 0.f;   // -1=真左 〜 1=真右
    if (distance > 1.f) pan = Math::Clamp((relative.x * right.x + relative.y * right.y) / distance, -1.f, 1.f);
    float gain = 1.f;  // 距離による音量
    if (spatial) {
        gain = (distance <= minDistance) ? 1.f : (distance >= maxDistance) ? 0.f : 1.f - (distance - minDistance) / (maxDistance - minDistance);
    }
    else {
        pan = 0.f;
    }

    wchar_t buf[200];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"音源 AudioSourceComponent: %ls   位置で聞く SetSpatial: %ls", source->IsPlaying() ? L"鳴っている" : L"止まっている", spatial ? L"ON" : L"OFF(どこでも同じ)");
    lines.push_back({ buf, blue });
    swprintf_s(buf, L"SetDistance:  最大音量 %.0f まで / 聞こえなくなる %.0f   (緑の輪・黄の輪)", minDistance, maxDistance);
    lines.push_back({ buf, yellow });
    swprintf_s(buf, L"聞く側までの距離 %.0f   距離による音量(目安) %.2f   音源の向き: %ls %.0f%%", distance, gain,
        (pan < -0.05f) ? L"左" : (pan > 0.05f) ? L"右" : L"真ん中", fabsf(pan) * 100.f);
    lines.push_back({ buf, orange });
    lines.push_back({ L"1 鳴/止   2 位置の効き   ↑↓ 聞こえなくなる距離   ←→ 回る速さ   SPACE その場所に1回鳴らす", green });
    DrawDemoStatus(lines, 1100.f);

    // 左右の目安の棒(音源が聞く側の左右どちらにあるか。位置を使わない時は真ん中)
    Image::DrawRect(gaugeCenter, gaugeSize, { 0.f, 0.f, 0.f, 0.7f });
    Image::DrawRect({ gaugeCenter.x, gaugeCenter.y }, { 2.f, gaugeSize.y + 8.f }, { 1.f, 1.f, 1.f, 0.8f });
    Image::DrawRect({ gaugeCenter.x + pan * gaugeSize.x * 0.5f, gaugeCenter.y }, { 14.f, gaugeSize.y + 8.f }, orange);
    Text::Draw(L"左", gaugeCenter.x - gaugeSize.x * 0.5f - 24.f, gaugeCenter.y, MakeDemoTextStyle(20.f, white, TextAlign::Center));
    Text::Draw(L"右", gaugeCenter.x + gaugeSize.x * 0.5f + 24.f, gaugeCenter.y, MakeDemoTextStyle(20.f, white, TextAlign::Center));
}
