#include "TransformDemo.h"
#include "../../Components/Core/TransformDemoController.h"
#include "../../Components/Core/SpinComponent.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

GameObject* CreateTransformDemo(Scene& scene, DemoMode mode) {
    const bool is3D = (mode == DemoMode::Mode3D);
    const float parentSize = 110.f;                                    // 親の大きさ(一辺)
    const DirectX::XMFLOAT3 childLocalPosition = { 1.6f, 0.f, 0.f };   // 子の位置(親から見た位置。親の大きさの1.6倍だけ右)
    const DirectX::XMFLOAT3 childLocalScale = is3D ? DirectX::XMFLOAT3{ 0.5f, 0.5f, 0.5f } : DirectX::XMFLOAT3{ 0.5f, 0.5f, 1.f };   // 子の大きさ(親の半分)

    // 親(大きい物)
    GameObject* parent = scene.CreateObject("Parent");
    parent->transform.scale = { parentSize, parentSize, is3D ? parentSize : 1.f };
    if (is3D) {
        parent->transform.position = { 0.f, 0.f, 300.f };
        parent->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(0.4f, 0.6f, 0.9f, 1.0f);
    }
    else {
        parent->transform.position = { 0.f, -20.f, 0.f };
        parent->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(5);
    }

    // 子(小さい物): 親につなぐ。位置と大きさは、親から見た値になる。自分でもゆっくり回る
    GameObject* child = scene.CreateObject("Child");
    child->transform.position = childLocalPosition;
    child->transform.scale = childLocalScale;
    if (is3D) child->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(1.0f, 0.8f, 0.3f, 1.0f);
    else child->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(2);
    child->transform.SetParent(&parent->transform);
    child->AddComponent<SpinComponent>(mode, 90.f);

    // キーの操作と状態の表示をする役
    GameObject* demo = scene.CreateObject("TransformDemo");
    demo->AddComponent<TransformDemoController>(mode, parent, child, parentSize, childLocalPosition, childLocalScale);
    return demo;
}
