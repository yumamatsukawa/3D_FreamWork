#include "ColliderDemoController.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoMouseGround.h"
#include "../Common/DemoStatus.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    constexpr size_t maxBodies = 30;        // 出せる数(超えたら古い物から消す)
    constexpr float bodySize2D = 50.f;      // 2D: 出す物の大きさ
    constexpr float bodySize3D = 70.f;      // 3D: 出す物の大きさ
    constexpr float spawnHeight3D = 500.f;  // 3D: 出す高さ
    constexpr float spawnLimit3D = 1400.f;  // 3D: 出せる範囲(原点から)
    constexpr float minSpeed2D = 250.f;     // 2D: 出す時の速さ(ランダム)
    constexpr float maxSpeed2D = 400.f;
    constexpr float bodyDrag2D = 0.3f;      // 2D: 空気抵抗(重力が無いので、ぶつかり続けて止まるまでの時間を短くする)
    constexpr float moverSpeed = 0.8f;      // 動く台が往復する速さ(1秒あたりの角度)
    constexpr float uiAreaY = 270.f;        // 画面の下のこれより下(マウスのY+=下)は、ボタンなので出さない
    constexpr int launchFrames = 2;         // 出してから、速さを入れるまでのフレーム数

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
}

void ColliderDemoController::Init() {
    // 当たり判定の枠線を出す(全体の設定。Uninitで戻す。ガイドの設定のリセットより後に呼ばれるよう、ガイドを先に作る)
    RigidbodyComponent::SetDebugDrawEnabled(debugDraw);
}

void ColliderDemoController::Uninit() {
    // 全体の設定を戻す
    RigidbodyComponent::SetDebugDrawEnabled(false);
}

const wchar_t* ColliderDemoController::GetShapeName() const {
    if (mode == DemoMode::Mode2D) {
        if (shape == DemoShape::Round) return L"Circle(円)";
        if (shape == DemoShape::Block) return L"Square(四角)";
        return L"Capsule2D(カプセル)";
    }
    if (shape == DemoShape::Round) return L"Sphere(球)";
    if (shape == DemoShape::Block) return L"Box(箱)";
    return L"Capsule(カプセル)";
}

void ColliderDemoController::ClearBodies() {
    for (const Spawned& item : spawned) item.object->Destroy();
    spawned.clear();
}

void ColliderDemoController::Spawn() {
    const bool is2D = (mode == DemoMode::Mode2D);
    const float size = is2D ? bodySize2D : bodySize3D;

    // 出す場所: 2Dはマウスの場所(壁の内側だけ)、3Dはマウスの下の地面の上
    DemoBodyDesc desc;
    desc.shape = shape;
    desc.type = BodyType::Dynamic;
    desc.size = size;
    desc.name = "DemoBody";
    DirectX::XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
    if (is2D) {
        const DirectX::XMFLOAT2 mouse = Input::GetMouseWorldPosition();
        if (mouse.x < arenaMin.x || mouse.x > arenaMax.x || mouse.y < arenaMin.y || mouse.y > arenaMax.y) return;
        // 物の半分の大きさだけ内側に寄せて、壁にめり込まないようにする(カプセルは縦に長い)
        const float halfHeight = (shape == DemoShape::Capsule) ? size : size * 0.5f;
        desc.position = { Math::Clamp(mouse.x, arenaMin.x + size * 0.5f, arenaMax.x - size * 0.5f),
                          Math::Clamp(mouse.y, arenaMin.y + halfHeight, arenaMax.y - halfHeight), 0.f };
        desc.color = { 1.f, 1.f, 1.f, 1.f };
        desc.cell = (shape == DemoShape::Capsule) ? 1 : 6;

        // 勢い: ランダムな向きと速さ
        const float angle = Math::RandomRange(0.f, 6.2832f);
        const float speed = Math::RandomRange(minSpeed2D, maxSpeed2D);
        velocity = { cosf(angle) * speed, sinf(angle) * speed, 0.f };
    }
    else {
        DirectX::XMFLOAT3 ground;
        if (!GetMouseGroundPoint(groundY, ground)) return;
        if (fabsf(ground.x) > spawnLimit3D || fabsf(ground.z) > spawnLimit3D) return;
        desc.position = { ground.x, spawnHeight3D, ground.z };
        if (shape == DemoShape::Round) desc.color = { 0.4f, 0.7f, 1.f, 1.f };
        else if (shape == DemoShape::Block) desc.color = { 1.f, 0.7f, 0.4f, 1.f };
        else desc.color = { 0.5f, 1.f, 0.5f, 1.f };
    }

    Scene* scene = GetOwner()->GetScene();
    GameObject* body = is2D ? CreateDemoBody2D(*scene, desc) : CreateDemoBody3D(*scene, desc);
    if (is2D) body->GetComponent<RigidbodyComponent>()->SetDrag(bodyDrag2D);
    spawned.push_back({ body, 0, velocity });

    // 多すぎたら、一番古い物を消す
    if (spawned.size() > maxBodies) {
        spawned.front().object->Destroy();
        spawned.erase(spawned.begin());
    }
}

void ColliderDemoController::LaunchSpawned() {
    // 出した直後は剛体が速さを0にするので、少し待ってから速さを入れる(2Dだけ。3Dは落とすだけなので、速さを触らない)
    if (mode != DemoMode::Mode2D) return;
    for (Spawned& item : spawned) {
        if (item.frames > launchFrames) continue;
        item.frames++;
        if (item.frames == launchFrames) {
            RigidbodyComponent* rigidbody = item.object->GetComponent<RigidbodyComponent>();
            if (rigidbody) rigidbody->SetVelocity(item.velocity);
        }
    }
}

void ColliderDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1・2・3キー: 出す形
    if (Input::GetKeyDown(KEY_1)) shape = DemoShape::Round;
    if (Input::GetKeyDown(KEY_2)) shape = DemoShape::Block;
    if (Input::GetKeyDown(KEY_3)) shape = DemoShape::Capsule;

    // F1キー: 当たり判定の枠線(全てのRigidbodyComponentに効く)
    if (Input::GetKeyDown(KEY_F1)) {
        debugDraw = !debugDraw;
        RigidbodyComponent::SetDebugDrawEnabled(debugDraw);
    }

    // Cキー: 出した物を全部消す
    if (Input::GetKeyDown(KEY_C)) ClearBodies();

    // 動く台(Kinematic): コードでTransformを動かす。Dynamicの物を押せる
    if (mover) {
        mover->transform.position.x = moverCenter.x + sinf(Time::GetTime() * moverSpeed) * moverRange;
    }

    // クリック: 物を出す(画面の下のボタンの上では出さない)
    if (Input::GetKeyDown(MOUSE_LEFT) && Input::GetMousePosition().y < uiAreaY) Spawn();

    LaunchSpawned();
}

void ColliderDemoController::Draw() {
    // 見本の名札
    for (const Label& label : labels) DrawDemoLabel(mode, label.position, label.text, 0.f);

    // 状態の一覧
    wchar_t buf[96];
    std::vector<DemoStatusLine> lines;
    lines.push_back({ std::wstring(L"[1][2][3] 出す形: ") + GetShapeName(), yellow });
    lines.push_back({ mode == DemoMode::Mode2D ? L"クリック: 勢いをつけて出す(重力は無い)" : L"クリック: 地面の上から落とす", white });
    lines.push_back({ std::wstring(L"[F1] 当たり判定の枠線: ") + (debugDraw ? L"ON(緑=通常 黄=トリガー)" : L"OFF"), green });
    swprintf_s(buf, L"出した数: %d / %d   [C] 全部消す", (int)spawned.size(), (int)maxBodies);
    lines.push_back({ buf, white });
    lines.push_back({ L"固定の見本=Static / 往復する台=Kinematic", cyan });
    DrawDemoStatus(lines);
}
