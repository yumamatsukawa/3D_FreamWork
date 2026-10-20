#include "DemoWall.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"

namespace {
    constexpr float wallDepth2D = 200.f;   // 2Dの壁の奥行き(Z固定の物がどこで当たっても、壁の中に入るよう厚くする)
}

GameObject* CreateDemoWall2D(Scene& scene, DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 size) {
    GameObject* wall = scene.CreateObject("DemoWall");
    wall->transform.position = { center.x, center.y, 1.f };   // zを1にして、zが0の物より奥に描く
    wall->transform.scale = { size.x, size.y, 1.f };

    // 見た目: 2Dモードのスプライト(少し白っぽい色合い)
    SpriteRenderer* sprite = wall->AddComponent<SpriteRenderer>(L"Assets/button.png");
    sprite->SetMode(SpriteMode::World2D);
    sprite->SetColor(0.8f, 0.85f, 1.f, 1.f);

    // 当たり判定(PhysX): 動かない板(奥行きは厚く)
    wall->AddComponent<BoxRigidbodyComponent>(BodyType::Static, DirectX::XMFLOAT3{ size.x, size.y, wallDepth2D });
    return wall;
}

void CreateDemoArena2D(Scene& scene, DirectX::XMFLOAT2 min, DirectX::XMFLOAT2 max, float thickness) {
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    const float centerX = (min.x + max.x) * 0.5f;
    const float centerY = (min.y + max.y) * 0.5f;

    // 下・上・左・右の順に置く(上下は左右の壁の厚みの分まで伸ばして、角にすき間を作らない)
    CreateDemoWall2D(scene, { centerX, min.y - thickness * 0.5f }, { width + thickness * 2.f, thickness });
    CreateDemoWall2D(scene, { centerX, max.y + thickness * 0.5f }, { width + thickness * 2.f, thickness });
    CreateDemoWall2D(scene, { min.x - thickness * 0.5f, centerY }, { thickness, height });
    CreateDemoWall2D(scene, { max.x + thickness * 0.5f, centerY }, { thickness, height });
}

GameObject* CreateDemoWall3D(Scene& scene, DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 size) {
    GameObject* wall = scene.CreateObject("DemoWall");
    wall->transform.position = center;
    wall->transform.scale = size;

    // 見た目(3Dメッシュの立方体)と、当たり判定(PhysX。動かない箱)
    wall->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(0.55f, 0.6f, 0.75f, 1.f);
    wall->AddComponent<BoxRigidbodyComponent>(BodyType::Static, size);
    return wall;
}
