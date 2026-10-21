#include "WorldTextDemo.h"
#include "../../Components/Common/DemoTextStyle.h"
#include "../../Components/UI/WorldTextDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Render3D/WorldTextRenderer.h"
#include <string>

namespace {
    constexpr float groundTop = -50.f;           // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float wallZ = 300.f;               // 壁の中心のZ
    constexpr float wallDepth = 30.f;            // 壁の厚み
    constexpr float signZ = wallZ - wallDepth * 0.5f - 1.f;   // 壁に貼る看板のZ(壁の表面の少し手前。カメラ側は -Z)
    constexpr int gridColumns = 10;              // 小さな看板の並べ方
    constexpr int gridRows = 6;
    constexpr float gridLeft = 120.f;            // 小さな看板の左端のX
    constexpr float gridTop = 250.f;             // 小さな看板の1行目のY
    constexpr float gridGapX = 70.f;
    constexpr float gridGapY = 38.f;

    // 箱を1つ置く(底が地面に着く位置のx, z。size: 大きさ)
    GameObject* CreateBox(Scene& scene, const char* name, float x, float bottomY, float z, DirectX::XMFLOAT3 size, DirectX::XMFLOAT4 color) {
        GameObject* object = scene.CreateObject(name);
        object->transform.position = { x, bottomY + size.y * 0.5f, z };
        object->transform.scale = size;
        object->AddComponent<MeshRenderer>(Mesh::CreateCube())->GetMaterial().color = color;
        return object;
    }

    // 看板を1枚置く(position: 板の中心。style: 文字の見た目)
    WorldTextRenderer* CreateSign(Scene& scene, const char* name, DirectX::XMFLOAT3 position, const wchar_t* text, const TextStyle& style) {
        GameObject* object = scene.CreateObject(name);
        object->transform.position = position;
        WorldTextRenderer* sign = object->AddComponent<WorldTextRenderer>();
        sign->SetText(text);
        sign->SetStyle(style);
        return sign;
    }
}

GameObject* CreateWorldTextDemo(Scene& scene) {
    // 壁(看板を貼る) と、壁の前の柱(柱の後ろの看板が隠れるのを見せる)
    CreateBox(scene, "Wall", 0.f, groundTop, wallZ, { 1700.f, 320.f, wallDepth }, { 0.55f, 0.5f, 0.45f, 1.0f });
    CreateBox(scene, "Pillar", 0.f, groundTop, 200.f, { 60.f, 240.f, 60.f }, { 0.45f, 0.5f, 0.6f, 1.0f });

    // 操作役
    GameObject* demo = scene.CreateObject("WorldTextDemo");
    auto* controller = demo->AddComponent<WorldTextDemoController>();

    // 壁の看板: 題名(縁取りは MakeDemoTextStyle の黒)、カウント、柱の後ろの看板
    WorldTextRenderer* title = CreateSign(scene, "TitleSign", { -450.f, 200.f, signZ }, L"ようこそ！ WorldText", MakeDemoTextStyle(64.f, { 1.0f, 0.9f, 0.4f, 1.0f }, TextAlign::Center));
    WorldTextRenderer* counter = CreateSign(scene, "CounterSign", { -450.f, 110.f, signZ }, L"Count: 0", MakeDemoTextStyle(56.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    WorldTextRenderer* hidden = CreateSign(scene, "HiddenSign", { 0.f, 130.f, signZ }, L"柱の後ろの看板", MakeDemoTextStyle(56.f, { 0.6f, 1.0f, 0.7f, 1.0f }, TextAlign::Center));
    controller->AddSign(title);
    controller->AddSign(counter);
    controller->AddSign(hidden);

    // 小さな看板60枚(文字は操作役が決める。全部同じ文字なら、テクスチャは1枚を共有する)
    TextStyle smallStyle = MakeDemoTextStyle(28.f, { 1.0f, 0.85f, 0.5f, 1.0f }, TextAlign::Center);
    for (int row = 0; row < gridRows; row++) {
        for (int column = 0; column < gridColumns; column++) {
            WorldTextRenderer* sign = CreateSign(scene, "SmallSign",
                { gridLeft + gridGapX * (float)column, gridTop - gridGapY * (float)row, signZ }, L"HELLO", smallStyle);
            controller->AddSign(sign, true);
        }
    }

    // 傾く看板: 柱の上の板(背景の色と余白を付けた看板。左右キーで回す。裏から見た時の見え方は D キーで変える)
    CreateBox(scene, "BoardPost", -350.f, groundTop, 80.f, { 14.f, 120.f, 14.f }, { 0.4f, 0.28f, 0.18f, 1.0f });
    GameObject* boardObject = scene.CreateObject("BoardSign");
    boardObject->transform.position = { -350.f, 100.f, 80.f };
    WorldTextRenderer* board = boardObject->AddComponent<WorldTextRenderer>();
    board->SetText(L"傾く看板");
    board->SetStyle(MakeDemoTextStyle(56.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    board->SetBackground({ 0.35f, 0.2f, 0.1f, 1.0f }, 14.f);
    controller->AddSign(board);

    // ビルボードの看板: いつもカメラの方を向く(B キーでオン/オフ)
    WorldTextRenderer* billboard = CreateSign(scene, "BillboardSign", { 250.f, 40.f, 80.f }, L"ビルボード", MakeDemoTextStyle(56.f, { 0.5f, 0.9f, 1.0f, 1.0f }, TextAlign::Center));
    controller->AddSign(billboard);

    controller->Setup(billboard, board, boardObject, counter);
    return demo;
}
