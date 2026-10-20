#include "RigidbodySettingsDemo.h"
#include "../Common/DemoBody.h"
#include "../Common/DemoWall.h"
#include "../../Components/Physics/RigidbodySettingsDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

namespace {
    // 2D版: 壁で囲んだ広い所に、上(A)と下(B)の箱を置く(上の壁は、説明の板の裏に隠れる高さ)
    const DirectX::XMFLOAT2 arenaMin2D = { -600.f, -250.f };      // 壁の内側の左下
    const DirectX::XMFLOAT2 arenaMax2D = { 600.f, 200.f };        // 壁の内側の右上
    constexpr float boxSize2D = 50.f;
    const DirectX::XMFLOAT3 startA2D = { -540.f, -45.f, 0.f };
    const DirectX::XMFLOAT3 startB2D = { -540.f, -170.f, 0.f };

    // 3D版: 地面の上。手前(A)と奥(B)のレーン
    constexpr float groundTop = -50.f;                            // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float wallX3D = 920.f;                              // 壁の中心のX(内側の面は±900)
    constexpr float boxSize3D = 80.f;
    const DirectX::XMFLOAT3 startA3D = { -700.f, groundTop + boxSize3D * 0.5f + 5.f, -170.f };
    const DirectX::XMFLOAT3 startB3D = { -700.f, groundTop + boxSize3D * 0.5f + 5.f, 170.f };

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

GameObject* CreateRigidbodySettingsDemo(Scene& scene, DemoMode mode) {
    // 操作役(設定の選択・蹴る・戻す)
    GameObject* demo = scene.CreateObject("RigidbodySettingsDemo");
    RigidbodySettingsDemoController* controller = demo->AddComponent<RigidbodySettingsDemoController>(mode);

    GameObject* boxA = nullptr;
    GameObject* boxB = nullptr;
    if (mode == DemoMode::Mode2D) {
        CreateDemoArena2D(scene, arenaMin2D, arenaMax2D);
        boxA = CreateBox(scene, mode, startA2D, boxSize2D, { 1.f, 1.f, 1.f, 1.f }, "BoxA");
        boxB = CreateBox(scene, mode, startB2D, boxSize2D, { 1.f, 1.f, 1.f, 1.f }, "BoxB");
    }
    else {
        CreateDemoWall3D(scene, { -wallX3D, 100.f, 0.f }, { 40.f, 400.f, 700.f });
        CreateDemoWall3D(scene, { wallX3D, 100.f, 0.f }, { 40.f, 400.f, 700.f });
        boxA = CreateBox(scene, mode, startA3D, boxSize3D, { 0.5f, 1.f, 0.5f, 1.f }, "BoxA");      // Aは緑
        boxB = CreateBox(scene, mode, startB3D, boxSize3D, { 1.f, 0.7f, 0.4f, 1.f }, "BoxB");      // Bはオレンジ
    }

    // 比べる2つの箱を操作役に渡す(剛体の設定は、操作役が決める)
    controller->Setup(boxA, boxB);
    return demo;
}
