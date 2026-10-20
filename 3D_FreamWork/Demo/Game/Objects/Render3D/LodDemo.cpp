#include "LodDemo.h"
#include "../../Components/Render3D/LodDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr int pillarCount = 15;          // 柱の並び(縦と横の本数)
    constexpr float spacing = 320.f;         // 柱の間隔
    constexpr float groundTop = -50.f;       // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float pillarRadius = 40.f;     // 柱の半径
    constexpr float pillarHeight = 240.f;    // 柱の高さ
    constexpr float drawDistance = 3000.f;   // これより遠い柱は描かない
    constexpr float shadowDistance = 2000.f; // これより遠い柱は影を落とさない
    constexpr float outlineDistance = 1500.f;// これより遠い柱は枠線を描かない
}

GameObject* CreateLodDemo(Scene& scene) {
    GameObject* demo = scene.CreateObject("LodDemo");
    LodDemoController* controller = demo->AddComponent<LodDemoController>();

    // 柱の形(全部同じ。三角形が多いカプセルにして、省いた時の差が分かるようにする)
    const std::vector<MeshVertex> vertices = Mesh::CreateCapsule(pillarRadius, pillarHeight, 6, 16);

    // 15x15本の柱を、原点を中心に並べる
    const float origin = -spacing * (float)(pillarCount - 1) * 0.5f;
    for (int row = 0; row < pillarCount; row++) {
        for (int column = 0; column < pillarCount; column++) {
            GameObject* pillar = scene.CreateObject("Pillar");
            pillar->transform.position = { origin + spacing * (float)column, groundTop + pillarHeight * 0.5f, origin + spacing * (float)row };

            // 見た目: 手前から奥へ色を変える(どの辺りが残っているか分かりやすい)
            MeshRenderer* renderer = pillar->AddComponent<MeshRenderer>(vertices);
            const float t = (float)row / (float)(pillarCount - 1);
            renderer->GetMaterial().color = { 0.9f - 0.6f * t, 0.5f, 0.3f + 0.6f * t, 1.0f };
            renderer->GetMaterial().specularStrength = 0.3f;
            renderer->GetMaterial().outlineColor = { 1.0f, 1.0f, 0.4f, 1.0f };
            renderer->GetMaterial().receiveShadow = true;

            // 距離で省く設定(描かない・影を落とさない・枠線を描かない距離)
            renderer->SetLod({ drawDistance, shadowDistance, outlineDistance });
            controller->AddPillar(renderer);
        }
    }
    return demo;
}
