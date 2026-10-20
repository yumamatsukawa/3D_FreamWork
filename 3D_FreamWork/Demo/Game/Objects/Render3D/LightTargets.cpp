#include "LightTargets.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr float groundTop = -50.f;      // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float sphereSize = 120.f;     // 球の直径
    constexpr float cubeSize = 110.f;       // 立方体の一辺
    constexpr float spacing = 200.f;        // 並べる間隔(X)
    constexpr float frontZ = 0.f;           // 手前の列(球)のZ
    constexpr float backZ = 300.f;          // 奥の列(立方体)のZ

    // 見本1つを置く(column: 左から何番目か。-2〜2)
    void CreateTarget(Scene& scene, int column, bool sphere, const Material& material) {
        const float size = sphere ? sphereSize : cubeSize;
        GameObject* target = scene.CreateObject("LightTarget");
        target->transform.position = { spacing * (float)column, groundTop + size * 0.5f, sphere ? frontZ : backZ };
        target->transform.scale = { size, size, size };
        target->AddComponent<MeshRenderer>(sphere ? Mesh::CreateSphere() : Mesh::CreateCube())->SetMaterial(material);
    }
}

void CreateLightTargets(Scene& scene) {
    // 白くて少しつやのある材質(平行光源・点光源・スポットライトの明るさとハイライトが見やすい)
    Material white;
    white.color = { 0.9f, 0.9f, 0.9f, 1.0f };
    white.specularStrength = 0.6f;
    white.shininess = 32.f;

    for (int column = -2; column <= 2; column++) {
        CreateTarget(scene, column, true, white);
        CreateTarget(scene, column, false, white);
    }
}
