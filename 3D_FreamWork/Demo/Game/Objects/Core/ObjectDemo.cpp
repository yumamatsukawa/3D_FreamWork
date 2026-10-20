#include "ObjectDemo.h"
#include "../../Components/Core/ObjectDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

GameObject* CreateObjectDemo(Scene& scene, DemoMode mode) {
    // 部品を付け外しされる中央の物(最初は何の部品も付いていない)
    GameObject* target = scene.CreateObject("ObjectTarget");
    if (mode == DemoMode::Mode3D) {
        target->transform.position = { 0.f, 0.f, 300.f };
        target->transform.scale = { 100.f, 100.f, 100.f };
        target->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(0.9f, 0.6f, 0.3f, 1.0f);
    }
    else {
        target->transform.position = { 0.f, -20.f, 0.f };
        target->transform.scale = { 140.f, 140.f, 1.f };
        target->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(4);
    }

    // キー・クリックの操作と状態の表示をする役
    GameObject* demo = scene.CreateObject("ObjectDemo");
    demo->AddComponent<ObjectDemoController>(mode, target);
    return demo;
}
