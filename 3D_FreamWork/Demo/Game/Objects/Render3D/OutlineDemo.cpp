#include "OutlineDemo.h"
#include "../../Components/Render3D/OutlineDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr float groundTop = -50.f;      // 地面の上面の高さ(DemoGroundと同じ)

    // 物を1つ置く(position: 底が地面に着く位置のx, z。size: 大きさ。色を付けて、立体が分かるようにハイライトも少し付ける)
    MeshRenderer* CreateShape(Scene& scene, const char* name, bool sphere, float x, float z, DirectX::XMFLOAT3 size, DirectX::XMFLOAT4 color) {
        GameObject* object = scene.CreateObject(name);
        object->transform.position = { x, groundTop + size.y * 0.5f, z };
        object->transform.scale = size;
        MeshRenderer* renderer = object->AddComponent<MeshRenderer>(sphere ? Mesh::CreateSphere() : Mesh::CreateCube());
        renderer->GetMaterial().color = color;
        renderer->GetMaterial().specularStrength = 0.4f;
        return renderer;
    }
}

GameObject* CreateOutlineDemo(Scene& scene) {
    // 左: 枠線を付ける立方体
    MeshRenderer* outlined = CreateShape(scene, "OutlinedCube", false, -450.f, 100.f, { 130.f, 130.f, 130.f }, { 0.6f, 0.6f, 0.7f, 1.0f });

    // 真ん中: 壁と、その後ろの球。球は壁より後に描く(drawPriorityを大きくする)ので、壁越しの枠線が壁に上書きされない
    CreateShape(scene, "Wall", false, 0.f, 0.f, { 320.f, 200.f, 40.f }, { 0.55f, 0.4f, 0.3f, 1.0f });
    MeshRenderer* hidden = CreateShape(scene, "HiddenSphere", true, 0.f, 250.f, { 120.f, 120.f, 120.f }, { 0.9f, 0.5f, 0.3f, 1.0f });
    hidden->GetOwner()->SetDrawPriority(5);

    // 右: 縁の光を付ける球(暗い色にして、縁の光が見えやすくする)
    MeshRenderer* rimmed = CreateShape(scene, "RimSphere", true, 450.f, 100.f, { 140.f, 140.f, 140.f }, { 0.3f, 0.3f, 0.35f, 1.0f });

    // 操作役(オン/オフ・太さ・強さ・名前の表示)
    GameObject* demo = scene.CreateObject("OutlineDemo");
    demo->AddComponent<OutlineDemoController>()->Setup(outlined, hidden, rimmed);
    return demo;
}
