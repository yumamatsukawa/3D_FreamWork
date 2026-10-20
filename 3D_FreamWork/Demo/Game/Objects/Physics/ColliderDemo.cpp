#include "ColliderDemo.h"
#include "../Common/DemoBody.h"
#include "../Common/DemoWall.h"
#include "../../Components/Physics/ColliderDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

namespace {
    // 2D版の舞台(画面の右側を、壁で囲む。左上は状態の板が出る)
    const DirectX::XMFLOAT2 arenaMin2D = { 40.f, -250.f };    // 壁の内側の左下
    const DirectX::XMFLOAT2 arenaMax2D = { 610.f, 190.f };    // 壁の内側の右上
    constexpr float specimenSize2D = 80.f;                    // 見本の大きさ
    constexpr float moverSize2D = 70.f;                       // 動く台の大きさ

    // 3D版の舞台
    constexpr float groundTop = -50.f;                        // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float specimenSize3D = 100.f;                   // 見本の大きさ
    constexpr float moverSize3D = 80.f;                       // 動く台の大きさ

    // 見本1つ(固定=Static)を作って、名札をつける
    void CreateSpecimen(Scene& scene, ColliderDemoController* controller, DemoMode mode, DemoShape shape,
        DirectX::XMFLOAT3 position, float size, const DirectX::XMFLOAT4& color, int cell, const wchar_t* label) {
        DemoBodyDesc desc;
        desc.shape = shape;
        desc.type = BodyType::Static;
        desc.position = position;
        desc.size = size;
        desc.color = color;
        desc.cell = cell;
        desc.name = "Specimen";
        GameObject* body = (mode == DemoMode::Mode2D) ? CreateDemoBody2D(scene, desc) : CreateDemoBody3D(scene, desc);

        // 名札は、物の上(カプセルは縦に長い)に出す
        const float height = (shape == DemoShape::Capsule) ? size * 2.f : size;
        controller->AddLabel({ body->transform.position.x, body->transform.position.y + height * 0.5f + 28.f, body->transform.position.z }, label);
    }

    // 2D版: 壁・見本3つ・動く台
    void Create2D(Scene& scene, ColliderDemoController* controller) {
        CreateDemoArena2D(scene, arenaMin2D, arenaMax2D);
        controller->SetArena2D(arenaMin2D, arenaMax2D);

        // 見本(床の上に並べる)。カプセルは縦に長いので、床に立たせる
        const float floorY = arenaMin2D.y;
        CreateSpecimen(scene, controller, DemoMode::Mode2D, DemoShape::Round, { 130.f, floorY + specimenSize2D * 0.5f, 0.f },
            specimenSize2D, { 1.f, 1.f, 1.f, 1.f }, 4, L"Circle");
        CreateSpecimen(scene, controller, DemoMode::Mode2D, DemoShape::Block, { 325.f, floorY + specimenSize2D * 0.5f, 0.f },
            specimenSize2D, { 1.f, 1.f, 1.f, 1.f }, 0, L"Square");
        CreateSpecimen(scene, controller, DemoMode::Mode2D, DemoShape::Capsule, { 520.f, floorY + specimenSize2D, 0.f },
            specimenSize2D, { 1.f, 1.f, 1.f, 1.f }, 2, L"Capsule2D");

        // 動く台(Kinematic): 左右に往復して、出した物を押す
        DemoBodyDesc desc;
        desc.shape = DemoShape::Block;
        desc.type = BodyType::Kinematic;
        desc.position = { 325.f, -30.f, 0.f };
        desc.size = moverSize2D;
        desc.color = { 1.f, 0.8f, 0.5f, 1.f };
        desc.name = "Mover";
        GameObject* mover = CreateDemoBody2D(scene, desc);
        controller->SetMover(mover, desc.position, 190.f);
        controller->AddLabel({ desc.position.x, desc.position.y + moverSize2D * 0.5f + 28.f, 0.f }, L"Kinematic");
    }

    // 3D版: 見本3つ・動く台
    void Create3D(Scene& scene, ColliderDemoController* controller) {
        controller->SetGroundY(groundTop);

        // 見本(地面の上に並べる。カプセルは縦に長いので、地面に立たせる)
        CreateSpecimen(scene, controller, DemoMode::Mode3D, DemoShape::Round, { -300.f, groundTop + specimenSize3D * 0.5f, 250.f },
            specimenSize3D, { 0.4f, 0.7f, 1.f, 1.f }, 0, L"Sphere");
        CreateSpecimen(scene, controller, DemoMode::Mode3D, DemoShape::Block, { 0.f, groundTop + specimenSize3D * 0.5f, 250.f },
            specimenSize3D, { 1.f, 0.7f, 0.4f, 1.f }, 0, L"Box");
        CreateSpecimen(scene, controller, DemoMode::Mode3D, DemoShape::Capsule, { 300.f, groundTop + specimenSize3D, 250.f },
            specimenSize3D, { 0.5f, 1.f, 0.5f, 1.f }, 0, L"Capsule");

        // 動く台(Kinematic): 左右に往復して、落ちてきた物を押す
        DemoBodyDesc desc;
        desc.shape = DemoShape::Block;
        desc.type = BodyType::Kinematic;
        desc.position = { 0.f, groundTop + moverSize3D * 0.5f, -150.f };
        desc.size = moverSize3D;
        desc.color = { 1.f, 0.85f, 0.4f, 1.f };
        desc.name = "Mover";
        GameObject* mover = CreateDemoBody3D(scene, desc);
        controller->SetMover(mover, desc.position, 400.f);
        controller->AddLabel({ desc.position.x, desc.position.y + moverSize3D * 0.5f + 40.f, desc.position.z }, L"Kinematic");
    }
}

GameObject* CreateColliderDemo(Scene& scene, DemoMode mode) {
    // 操作役(クリックで物を出す・枠線のオン/オフ)
    GameObject* demo = scene.CreateObject("ColliderDemo");
    ColliderDemoController* controller = demo->AddComponent<ColliderDemoController>(mode);

    if (mode == DemoMode::Mode2D) Create2D(scene, controller);
    else Create3D(scene, controller);
    return demo;
}
