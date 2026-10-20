#include "TriggerDemo.h"
#include "../Common/DemoBody.h"
#include "../Common/DemoWall.h"
#include "../../Components/Physics/TouchCounterController.h"
#include "../../Components/Physics/TriggerDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

namespace {
    // 2D版: 壁で囲んだ中に、左にトリガー、右に箱、真ん中に球
    const DirectX::XMFLOAT2 arenaMin2D = { -600.f, -250.f };      // 壁の内側の左下
    const DirectX::XMFLOAT2 arenaMax2D = { 600.f, 30.f };         // 壁の内側の右上
    constexpr float zoneSize2D = 200.f;                           // トリガー(丸)の直径
    constexpr float solidSize2D = 160.f;                          // 箱の一辺
    constexpr float moverSize2D = 60.f;                           // 球の直径

    // 3D版: 地面の上に、左にトリガー、右に箱、真ん中に球
    constexpr float groundTop = -50.f;                            // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float zoneSize3D = 300.f;
    constexpr float solidSize3D = 200.f;
    constexpr float moverSize3D = 80.f;

    // 動かない物(トリガーか箱)を作る。球が当たった回数を数える役(TouchCounterController)を付ける
    GameObject* CreateStatic(Scene& scene, DemoMode mode, DemoShape shape, DirectX::XMFLOAT3 position, float size,
        bool isTrigger, const DirectX::XMFLOAT4& color, int cell, const char* name) {
        DemoBodyDesc desc;
        desc.shape = shape;
        desc.type = BodyType::Static;
        desc.position = position;
        desc.size = size;
        desc.isTrigger = isTrigger;
        desc.color = color;
        desc.cell = cell;
        desc.name = name;
        GameObject* body = (mode == DemoMode::Mode2D) ? CreateDemoBody2D(scene, desc) : CreateDemoBody3D(scene, desc);
        body->AddComponent<TouchCounterController>("Mover");   // タグが Mover の物(球)が当たった時だけ数える
        return body;
    }
}

GameObject* CreateTriggerDemo(Scene& scene, DemoMode mode, GameObject* camera) {
    // 操作役(球を動かす・回数の表示)
    GameObject* demo = scene.CreateObject("TriggerDemo");
    TriggerDemoController* controller = demo->AddComponent<TriggerDemoController>(mode);

    const bool is2D = (mode == DemoMode::Mode2D);
    GameObject* zone = nullptr;
    GameObject* solid = nullptr;
    GameObject* mover = nullptr;

    // 球(動く物。タグ Mover)。3Dは転がらないよう、回転を固定する
    DemoBodyDesc moverDesc;
    moverDesc.shape = DemoShape::Round;
    moverDesc.type = BodyType::Dynamic;
    moverDesc.name = "Mover";
    moverDesc.tag = "Mover";

    if (is2D) {
        CreateDemoArena2D(scene, arenaMin2D, arenaMax2D);
        zone = CreateStatic(scene, mode, DemoShape::Round, { -320.f, -140.f, 0.f }, zoneSize2D, true, { 1.f, 1.f, 1.f, 1.f }, 2, "Zone");
        solid = CreateStatic(scene, mode, DemoShape::Block, { 320.f, -140.f, 0.f }, solidSize2D, false, { 1.f, 1.f, 1.f, 1.f }, 0, "Solid");
        moverDesc.position = { 0.f, -140.f, 0.f };
        moverDesc.size = moverSize2D;
        moverDesc.cell = 4;
        mover = CreateDemoBody2D(scene, moverDesc);
    }
    else {
        zone = CreateStatic(scene, mode, DemoShape::Round, { -380.f, groundTop + zoneSize3D * 0.25f, 0.f }, zoneSize3D, true, { 1.f, 0.9f, 0.3f, 1.f }, 0, "Zone");
        solid = CreateStatic(scene, mode, DemoShape::Block, { 380.f, groundTop + solidSize3D * 0.5f, 0.f }, solidSize3D, false, { 0.4f, 0.6f, 1.f, 1.f }, 0, "Solid");
        moverDesc.position = { 0.f, groundTop + moverSize3D * 0.5f + 5.f, 0.f };
        moverDesc.size = moverSize3D;
        moverDesc.color = { 0.5f, 1.f, 0.5f, 1.f };
        mover = CreateDemoBody3D(scene, moverDesc);
        mover->GetComponent<RigidbodyComponent>()->SetFreezeRotation(true);
    }

    // 動かす球・トリガー・箱を操作役に渡す
    controller->Setup(mover, zone, solid, camera);
    return demo;
}
