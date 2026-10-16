#include "Lantern.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/LightComponent.h"
#include "../Components/LanternController.h"

GameObject* CreateLantern(Scene& scene, DirectX::XMFLOAT3 center) {
    GameObject* lantern = scene.CreateObject("Lantern");
    lantern->transform.position = center;
    lantern->transform.scale    = { 30.0f, 30.0f, 30.0f };

    // 見た目(光る小さな球。自分は光の影響を受けず、いつも同じ明るさ)
    auto* meshRenderer = lantern->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    meshRenderer->SetColor(1.0f, 0.85f, 0.5f, 1.0f);
    meshRenderer->SetUnlit(true);

    // 周りを照らす光(点光源。オレンジ色)
    auto* light = lantern->AddComponent<LightComponent>(LightType::Point);
    light->SetColor(1.0f, 0.7f, 0.35f);
    light->SetIntensity(1.5f);
    light->SetRange(800.0f);

    // 動き
    auto* controller = lantern->AddComponent<LanternController>();
    controller->Setup(center);

    return lantern;
}
