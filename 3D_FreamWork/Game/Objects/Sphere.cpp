#include "Sphere.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"

GameObject* CreateSphere(Scene& scene) {
    GameObject* sphere = scene.CreateObject("Sphere");
    sphere->transform.position = { 150.0f, 100.0f,   0.0f };
    sphere->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 見た目(3Dメッシュ)
    auto* meshRenderer = sphere->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    meshRenderer->SetTexture(L"Assets/grid.png");

    return sphere;
}
