#include "ForceDemo.h"
#include "../Common/DemoBody.h"
#include "../Common/DemoWall.h"
#include "../../Components/Physics/ForceDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

namespace {
    // 2D版: 画面の左右に壁。上のレーンに軽い箱、下のレーンに重い箱
    constexpr float wallX2D = 615.f;                 // 壁の中心のX(内側の面は±600)
    constexpr float lightSize2D = 50.f;
    constexpr float heavySize2D = 90.f;
    const DirectX::XMFLOAT3 lightStart2D = { -540.f, -30.f, 0.f };
    const DirectX::XMFLOAT3 heavyStart2D = { -540.f, -150.f, 0.f };

    // 3D版: 左右に壁。手前のレーンに軽い箱、奥のレーンに重い箱(宙に浮かせる)
    constexpr float wallX3D = 920.f;                 // 壁の中心のX(内側の面は±900)
    constexpr float lightSize3D = 80.f;
    constexpr float heavySize3D = 120.f;
    const DirectX::XMFLOAT3 lightStart3D = { -700.f, 100.f, -170.f };
    const DirectX::XMFLOAT3 heavyStart3D = { -700.f, 100.f, 170.f };

    // 箱を1つ作る(回転できる。色は3Dだけ。2Dはボタンの絵の色のまま)
    GameObject* CreateBox(Scene& scene, DemoMode mode, DirectX::XMFLOAT3 position, float size, const DirectX::XMFLOAT4& color, const char* name) {
        DemoBodyDesc desc;
        desc.shape = DemoShape::Block;
        desc.type = BodyType::Dynamic;
        desc.position = position;
        desc.size = size;
        desc.canRotate = true;
        desc.name = name;
        desc.color = color;
        return (mode == DemoMode::Mode2D) ? CreateDemoBody2D(scene, desc) : CreateDemoBody3D(scene, desc);
    }
}

GameObject* CreateForceDemo(Scene& scene, DemoMode mode) {
    // 操作役(力の種類・SPACEで押す・V/T/R)
    GameObject* demo = scene.CreateObject("ForceDemo");
    ForceDemoController* controller = demo->AddComponent<ForceDemoController>(mode);

    GameObject* lightBox = nullptr;
    GameObject* heavyBox = nullptr;
    if (mode == DemoMode::Mode2D) {
        CreateDemoWall2D(scene, { -wallX2D, -110.f }, { 30.f, 260.f });
        CreateDemoWall2D(scene, { wallX2D, -110.f }, { 30.f, 260.f });
        lightBox = CreateBox(scene, mode, lightStart2D, lightSize2D, { 1.f, 1.f, 1.f, 1.f }, "LightBox");
        heavyBox = CreateBox(scene, mode, heavyStart2D, heavySize2D, { 1.f, 1.f, 1.f, 1.f }, "HeavyBox");
    }
    else {
        CreateDemoWall3D(scene, { -wallX3D, 100.f, 0.f }, { 40.f, 400.f, 700.f });
        CreateDemoWall3D(scene, { wallX3D, 100.f, 0.f }, { 40.f, 400.f, 700.f });
        lightBox = CreateBox(scene, mode, lightStart3D, lightSize3D, { 1.f, 0.7f, 0.4f, 1.f }, "LightBox");    // 軽い箱はオレンジ
        heavyBox = CreateBox(scene, mode, heavyStart3D, heavySize3D, { 0.4f, 0.6f, 1.f, 1.f }, "HeavyBox");    // 重い箱は青
    }

    // 動かす箱を操作役に渡す(質量・空気抵抗・固定は、操作役が決める)
    controller->Setup(lightBox, heavyBox);
    return demo;
}
