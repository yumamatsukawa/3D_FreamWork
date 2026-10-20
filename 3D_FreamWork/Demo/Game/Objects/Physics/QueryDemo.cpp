#include "QueryDemo.h"
#include "../Common/DemoBody.h"
#include "../../Components/Physics/QueryDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

namespace {
    // 調べられる物1つ分(Target か Decoy)の置き方
    struct Placement {
        DemoShape shape;
        float x, y;        // 2D: ワールドのX・Y。3D: ワールドのX・Z
        float size;
        bool target;       // true=Target(青・緑)、false=Decoy(オレンジ・赤)
    };

    // 2D版の並び(画面の左下の出発点から、いろいろな向きへ線を飛ばせる)
    const Placement placements2D[] = {
        { DemoShape::Round,   -380.f,  -60.f, 70.f, true },
        { DemoShape::Block,   -120.f, -110.f, 70.f, true },
        { DemoShape::Capsule,  100.f,  -40.f, 60.f, true },
        { DemoShape::Round,    410.f, -185.f, 80.f, true },
        { DemoShape::Block,    540.f,  -20.f, 60.f, true },
        { DemoShape::Round,   -230.f, -130.f, 60.f, false },
        { DemoShape::Round,      0.f, -120.f, 60.f, false },
        { DemoShape::Capsule,  300.f, -100.f, 50.f, false },
        { DemoShape::Round,    470.f,  -90.f, 70.f, false },
    };
    const DirectX::XMFLOAT3 origin2D = { -580.f, -230.f, 0.f };    // Raycast の出発点
    const DirectX::XMFLOAT3 zone2D = { 180.f, -180.f, 0.f };       // トリガーの位置
    constexpr float zoneSize2D = 120.f;

    // 3D版の並び(x, z)
    const Placement placements3D[] = {
        { DemoShape::Round,   -450.f,  200.f, 100.f, true },
        { DemoShape::Block,   -150.f, -100.f, 100.f, true },
        { DemoShape::Capsule,  150.f,  250.f,  80.f, true },
        { DemoShape::Round,    450.f,    0.f, 100.f, true },
        { DemoShape::Block,    650.f,  300.f,  90.f, true },
        { DemoShape::Round,   -250.f,   50.f,  80.f, false },
        { DemoShape::Capsule,  100.f,  -40.f,  70.f, false },
        { DemoShape::Round,    330.f,  200.f,  90.f, false },
        { DemoShape::Block,    520.f, -250.f,  80.f, false },
    };
    constexpr float groundTop = -50.f;                              // 地面の上面の高さ(DemoGroundと同じ)
    const DirectX::XMFLOAT3 origin3D = { -800.f, 0.f, -250.f };     // Raycast の出発点(線は、この高さで水平に飛ぶ)
    constexpr float zoneSize3D = 160.f;
    const DirectX::XMFLOAT2 zone3D = { 0.f, 350.f };                // トリガーの位置(x, z)

    // 調べられる物を1つ作る(動かない。Target か Decoy のタグを付ける)
    GameObject* CreateTarget(Scene& scene, DemoMode mode, const Placement& placement, int number) {
        const bool is2D = (mode == DemoMode::Mode2D);
        DemoBodyDesc desc;
        desc.shape = placement.shape;
        desc.type = BodyType::Static;
        desc.size = placement.size;
        desc.tag = placement.target ? "Target" : "Decoy";
        desc.name = desc.tag + std::to_string(number);

        if (is2D) {
            desc.position = { placement.x, placement.y, 0.f };
            desc.cell = placement.target ? 6 : 1;     // Target=青、Decoy=オレンジ
            return CreateDemoBody2D(scene, desc);
        }

        // 3D: 地面の上に置く(カプセルは縦に長い)。色は Target=青・緑、Decoy=オレンジ・赤
        const float halfHeight = (placement.shape == DemoShape::Capsule) ? placement.size : placement.size * 0.5f;
        desc.position = { placement.x, groundTop + halfHeight, placement.y };
        if (placement.target) desc.color = (placement.shape == DemoShape::Block) ? DirectX::XMFLOAT4{ 0.4f, 0.6f, 1.f, 1.f } : DirectX::XMFLOAT4{ 0.4f, 0.9f, 0.6f, 1.f };
        else desc.color = (placement.shape == DemoShape::Block) ? DirectX::XMFLOAT4{ 1.f, 0.4f, 0.4f, 1.f } : DirectX::XMFLOAT4{ 1.f, 0.7f, 0.3f, 1.f };
        return CreateDemoBody3D(scene, desc);
    }
}

GameObject* CreateQueryDemo(Scene& scene, DemoMode mode) {
    // 操作役(問い合わせの選択・結果の表示)
    GameObject* demo = scene.CreateObject("QueryDemo");
    QueryDemoController* controller = demo->AddComponent<QueryDemoController>(mode);

    // 調べられる物(Target 5つ・Decoy 4つ)
    const bool is2D = (mode == DemoMode::Mode2D);
    const Placement* placements = is2D ? placements2D : placements3D;
    const int count = is2D ? (int)(sizeof(placements2D) / sizeof(placements2D[0])) : (int)(sizeof(placements3D) / sizeof(placements3D[0]));
    int targetNumber = 0, decoyNumber = 0;
    for (int i = 0; i < count; i++) {
        const int number = placements[i].target ? ++targetNumber : ++decoyNumber;
        controller->AddTarget(CreateTarget(scene, mode, placements[i], number));
    }

    // トリガー(黄色い半透明。すり抜ける。タグ Target)。QueryTriggers::Include の時だけ見つかる
    DemoBodyDesc zone;
    zone.shape = DemoShape::Round;
    zone.type = BodyType::Static;
    zone.isTrigger = true;
    zone.tag = "Target";
    zone.name = "Zone";
    if (is2D) {
        zone.position = zone2D;
        zone.size = zoneSize2D;
        zone.cell = 2;    // 黄色
    }
    else {
        zone.position = { zone3D.x, groundTop + zoneSize3D * 0.5f, zone3D.y };
        zone.size = zoneSize3D;
        zone.color = { 1.f, 0.9f, 0.3f, 1.f };
    }
    controller->AddTarget(is2D ? CreateDemoBody2D(scene, zone) : CreateDemoBody3D(scene, zone));

    // Raycast の出発点と、3D版の線・範囲の高さ
    controller->SetOrigin(is2D ? origin2D : origin3D);
    controller->SetQueryHeight(origin3D.y);
    return demo;
}
