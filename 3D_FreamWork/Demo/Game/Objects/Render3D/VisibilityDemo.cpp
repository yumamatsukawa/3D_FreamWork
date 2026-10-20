#include "VisibilityDemo.h"
#include "../../Components/Render3D/VisibilityDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    using Shape = VisibilityDemoController::Shape;

    // 2D版の並び
    constexpr int columns2D = 11;           // 横の個数
    constexpr int rows2D = 9;               // 縦の個数
    constexpr float spacing2D = 300.f;      // 間隔
    constexpr float spriteSize = 60.f;      // 箱の画像の大きさ

    // 3D版の並び
    constexpr int count3D = 9;              // 縦と横の個数
    constexpr float spacing3D = 400.f;      // 間隔
    constexpr float groundTop = -50.f;      // 地面の上面の高さ(DemoGroundと同じ)
    constexpr float sphereSize = 120.f;     // 球の直径
    constexpr float cubeSize = 100.f;       // 立方体の一辺

    // 2D版: 箱の画像を並べる(偶数番目は円、奇数番目は四角で画面判定する)
    void CreateItems2D(Scene& scene, VisibilityDemoController* controller) {
        for (int row = 0; row < rows2D; row++) {
            for (int column = 0; column < columns2D; column++) {
                GameObject* object = scene.CreateObject("VisibilityBox");
                object->transform.position = { spacing2D * (float)(column - columns2D / 2), spacing2D * (float)(row - rows2D / 2), 0.f };
                object->transform.scale = { spriteSize, spriteSize, 1.f };

                // 見た目: 2Dモードの箱の画像(位置で色を変えて、動いた時に分かりやすくする)
                SpriteRenderer* sprite = object->AddComponent<SpriteRenderer>(L"Assets/box.png");
                sprite->SetMode(SpriteMode::World2D);
                sprite->SetColor(0.4f + 0.6f * (float)column / (float)(columns2D - 1), 0.9f - 0.5f * (float)row / (float)(rows2D - 1), 0.6f, 1.f);

                const bool round = ((row + column) % 2 == 0);
                controller->AddItem(object, sprite, nullptr, round ? Shape::Round : Shape::Square, spriteSize * 0.5f);
            }
        }
    }

    // 3D版: 球と立方体を並べる(球は球で、立方体は箱で画面判定する)
    void CreateItems3D(Scene& scene, VisibilityDemoController* controller) {
        const float origin = -spacing3D * (float)(count3D - 1) * 0.5f;
        for (int row = 0; row < count3D; row++) {
            for (int column = 0; column < count3D; column++) {
                const bool round = ((row + column) % 2 == 0);
                const float size = round ? sphereSize : cubeSize;
                GameObject* object = scene.CreateObject("VisibilityShape");
                object->transform.position = { origin + spacing3D * (float)column, groundTop + size * 0.5f, origin + spacing3D * (float)row };
                object->transform.scale = { size, size, size };

                // 見た目: 位置で色を変える
                MeshRenderer* mesh = object->AddComponent<MeshRenderer>(round ? Mesh::CreateSphere() : Mesh::CreateCube());
                mesh->SetColor(0.4f + 0.6f * (float)column / (float)(count3D - 1), 0.9f - 0.5f * (float)row / (float)(count3D - 1), 0.6f, 1.f);

                controller->AddItem(object, nullptr, mesh, round ? Shape::Round : Shape::Square, size * 0.5f);
            }
        }
    }
}

GameObject* CreateVisibilityDemo(Scene& scene, DemoMode mode, GameObject* camera) {
    // 操作役(画面に映っているかの判定・描かない設定・地図)
    GameObject* demo = scene.CreateObject("VisibilityDemo");
    VisibilityDemoController* controller = demo->AddComponent<VisibilityDemoController>(mode);

    if (mode == DemoMode::Mode2D) {
        CreateItems2D(scene, controller);
    }
    else {
        controller->SetCameraTarget(camera);
        CreateItems3D(scene, controller);
    }
    return demo;
}
