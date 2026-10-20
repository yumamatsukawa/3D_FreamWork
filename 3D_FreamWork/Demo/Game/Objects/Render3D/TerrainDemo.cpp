#include "TerrainDemo.h"
#include "../../Components/Render3D/TerrainDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr float probeSize = 60.f;   // 玉の直径
}

GameObject* CreateTerrainDemo(Scene& scene, TerrainRenderer* terrain) {
    GameObject* probe = scene.CreateObject("TerrainProbe");
    probe->transform.scale = { probeSize, probeSize, probeSize };

    // 見た目(光る黄色い玉。光の影響を受けない)
    auto* meshRenderer = probe->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    meshRenderer->SetColor(1.0f, 0.9f, 0.2f, 1.0f);
    meshRenderer->SetUnlit(true);

    // 動きと表示、と、調べる地形
    probe->AddComponent<TerrainDemoController>()->Setup(terrain);
    return probe;
}
