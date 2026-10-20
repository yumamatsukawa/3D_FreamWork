#include "CoordDemo.h"
#include "../../Components/Render/CoordDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"

namespace {
    // 3D版の立方体1つ分の設定(名前・位置(x, z)・大きさ・色)
    struct CubeSpec {
        const char* name;
        float x, z;
        float size;
        DirectX::XMFLOAT4 color;
    };
    const CubeSpec cubeSpecs[5] = {
        { "Red",    -450.f, 300.f, 100.f, { 0.85f, 0.25f, 0.25f, 1.0f } },
        { "Green",  -150.f, 450.f, 140.f, { 0.30f, 0.75f, 0.35f, 1.0f } },
        { "Blue",    150.f, 300.f, 100.f, { 0.30f, 0.45f, 0.90f, 1.0f } },
        { "Yellow",  450.f, 450.f, 120.f, { 0.90f, 0.80f, 0.30f, 1.0f } },
        { "Purple",    0.f, 700.f, 160.f, { 0.65f, 0.40f, 0.85f, 1.0f } },
    };
    constexpr float groundTop = -50.f;   // 地面の上面の高さ(DemoGroundと同じ)
}

GameObject* CreateCoordDemo(Scene& scene, DemoMode mode) {
    // 操作役
    GameObject* demo = scene.CreateObject("CoordDemo");
    CoordDemoController* controller = demo->AddComponent<CoordDemoController>(mode);

    if (mode == DemoMode::Mode2D) {
        // ワールドの同じ高さ(y)で、奥行き(z)だけが違う3つ(手前 → 奥)。zが違っても、大きさも画面の高さも同じに見える
        const float depths[3] = { -250.f, 0.f, 500.f };
        for (int i = 0; i < 3; i++) {
            GameObject* sprite = scene.CreateObject("DepthSprite");
            sprite->transform.position = { 60.f + 190.f * (float)i, -60.f, depths[i] };
            sprite->transform.scale = { 70.f, 70.f, 1.f };
            SpriteRenderer* renderer = sprite->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
            renderer->SetSpriteIndex(i * 4);
            renderer->SetMode(SpriteMode::World2D);   // 2Dモード: zが違っても大きさは同じ(zは前後だけに使う)
            controller->AddDepthSprite(sprite);
        }
    }
    else {
        // 調べられる立方体(Raycastに当たるよう、動かない当たり判定を付ける)
        for (const CubeSpec& spec : cubeSpecs) {
            GameObject* cube = scene.CreateObject(spec.name);
            cube->transform.position = { spec.x, groundTop + spec.size * 0.5f, spec.z };
            cube->transform.scale = { spec.size, spec.size, spec.size };
            MeshRenderer* mesh = cube->AddComponent<MeshRenderer>(Mesh::CreateCube());
            mesh->SetColor(spec.color.x, spec.color.y, spec.color.z, spec.color.w);
            cube->AddComponent<BoxRigidbodyComponent>(BodyType::Static, cube->transform.scale);
            controller->AddTarget(cube, mesh);
        }
    }
    return demo;
}
