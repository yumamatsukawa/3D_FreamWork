#include "DemoBody.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr float triggerAlpha = 0.6f;   // トリガーの半透明の濃さ(暗い背景でも見える濃さ)
}

GameObject* CreateDemoBody2D(Scene& scene, const DemoBodyDesc& desc) {
    // 大きさ(カプセルは縦長)
    const float width = desc.size;
    const float height = (desc.shape == DemoShape::Capsule) ? desc.size * 2.f : desc.size;

    GameObject* body = scene.CreateObject(desc.name, desc.tag);
    body->transform.position = { desc.position.x, desc.position.y, 0.f };
    body->transform.scale = { width, height, 1.f };

    // 見た目: 円・カプセルは丸い絵(box.png の1コマ)、四角はボタンの絵。2Dモードで、zが0の手前に描く(背景より手前)
    SpriteRenderer* sprite = nullptr;
    if (desc.shape == DemoShape::Block) {
        sprite = body->AddComponent<SpriteRenderer>(L"Assets/button.png");
        sprite->SetColor(desc.color.x, desc.color.y, desc.color.z, desc.isTrigger ? triggerAlpha : 1.f);
    }
    else {
        sprite = body->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
        sprite->SetSpriteIndex(desc.cell);
        sprite->SetColor(1.f, 1.f, 1.f, desc.isTrigger ? triggerAlpha : 1.f);
    }
    sprite->SetMode(SpriteMode::World2D);

    // 当たり判定+物理演算(PhysX): 形ごとの2D用のコライダー
    RigidbodyComponent* rigidbody = nullptr;
    switch (desc.shape) {
    case DemoShape::Round:
        rigidbody = body->AddComponent<CircleRigidbodyComponent>(desc.type, desc.size * 0.5f, 1.f, desc.isTrigger);
        break;
    case DemoShape::Block:
        rigidbody = body->AddComponent<SquareRigidbodyComponent>(desc.type, desc.size, 1.f, desc.isTrigger);
        break;
    case DemoShape::Capsule:
        rigidbody = body->AddComponent<Capsule2DRigidbodyComponent>(desc.type, desc.size * 0.5f, height, 1.f, desc.isTrigger, false, CapsuleAxis::Y);
        break;
    }

    // 動く物は、重力オフ(上から見た2D)・Z固定(奥に動かさない)。回転は、許した時以外は固定する
    if (desc.type == BodyType::Dynamic) {
        rigidbody->SetUseGravity(false);
        rigidbody->SetFreezePositionZ(true);
        if (!desc.canRotate) rigidbody->SetFreezeRotation(true);
    }
    return body;
}

GameObject* CreateDemoBody3D(Scene& scene, const DemoBodyDesc& desc) {
    GameObject* body = scene.CreateObject(desc.name, desc.tag);
    body->transform.position = desc.position;

    // 見た目(3Dメッシュ)と、当たり判定+物理演算(PhysX)を、形ごとに作る
    MeshRenderer* mesh = nullptr;
    switch (desc.shape) {
    case DemoShape::Round:
        body->transform.scale = { desc.size, desc.size, desc.size };
        mesh = body->AddComponent<MeshRenderer>(Mesh::CreateSphere());
        body->AddComponent<SphereRigidbodyComponent>(desc.type, desc.size * 0.5f, 1.f, desc.isTrigger);
        break;
    case DemoShape::Block:
        body->transform.scale = { desc.size, desc.size, desc.size };
        mesh = body->AddComponent<MeshRenderer>(Mesh::CreateCube());
        body->AddComponent<BoxRigidbodyComponent>(desc.type, DirectX::XMFLOAT3{ desc.size, desc.size, desc.size }, 1.f, desc.isTrigger);
        break;
    case DemoShape::Capsule:
        // カプセルのメッシュは実際の大きさで作るので、scaleは1のまま
        mesh = body->AddComponent<MeshRenderer>(Mesh::CreateCapsule(desc.size * 0.5f, desc.size * 2.f));
        body->AddComponent<CapsuleRigidbodyComponent>(desc.type, desc.size * 0.5f, desc.size * 2.f, 1.f, desc.isTrigger, false, CapsuleAxis::Y);
        break;
    }

    // 色。トリガーは半透明にして、後ろの物を隠さない
    mesh->SetColor(desc.color.x, desc.color.y, desc.color.z, desc.isTrigger ? triggerAlpha : desc.color.w);
    if (desc.isTrigger) mesh->SetDepthWrite(false);
    return body;
}
