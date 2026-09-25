#include "Sphere.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include "../../Engine/RigidbodyComponent.h"

GameObject* CreateSphere(Scene& scene) {
    GameObject* sphere = scene.CreateObject("Sphere");
    sphere->transform.position = { 0.0f, 100.0f,   0.0f };
    sphere->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 見た目(3Dメッシュ)
    auto* meshRenderer = sphere->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    meshRenderer->SetTexture(L"Assets/grid.png");

    // 物理演算(PhysX): 重力で落ちて、Groundの上に着地する
    auto* rigidbody = sphere->AddComponent<SphereRigidbodyComponent>(BodyType::Dynamic);
    rigidbody->SetFreezeRotation(true);

    return sphere;
}
