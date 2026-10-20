#include "EaseRunner.h"
#include "../../Components/Render/EaseRunnerController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    // 2D版: 上から下へ並べる
    constexpr float firstRowY = 185.f;   // 1行目のY(説明の板の下)
    constexpr float rowGap = 58.f;       // 行の間隔

    // 3D版: 左から右へ並べる(レーンは手前から奥へ伸びる)
    constexpr float laneGap = 140.f;     // レーンの間隔(X)
    constexpr float cubeSize = 60.f;     // 立方体の一辺
    constexpr float trackY = -48.f;      // 線の高さ(地面のすぐ上)

    // 2D版の1行
    GameObject* CreateEaseRunner2D(Scene& scene, const std::wstring& easeName, EaseFunction ease, int row) {
        const float y = firstRowY - rowGap * (float)row;

        // 動く範囲を示す線(丸より先に描く)
        GameObject* track = scene.CreateObject("EaseTrack");
        track->transform.position = { 0.f, y, 0.f };
        track->transform.scale = { 600.f, 4.f, 1.f };
        track->SetDrawPriority(-1);
        auto* trackSprite = track->AddComponent<SpriteRenderer>(L"Assets/button.png");
        trackSprite->SetColor(0.35f, 0.4f, 0.55f, 1.0f);

        // 動く丸(Assets/box.png は3x3コマの色付きの丸。行ごとに違うコマ)
        GameObject* runner = scene.CreateObject("EaseRunner");
        runner->transform.position = { -300.f, y, 0.f };
        runner->transform.scale = { 44.f, 44.f, 1.f };
        auto* sprite = runner->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
        sprite->SetSpriteIndex(row % 9);

        // 動かす役と、その設定をする役(動かす役を先に付ける)
        runner->AddComponent<TweenComponent>();
        runner->AddComponent<EaseRunnerController>(easeName, ease, DemoMode::Mode2D, row);
        return runner;
    }

    // 3D版の1本(立方体が手前から奥へ動く)
    GameObject* CreateEaseRunner3D(Scene& scene, const std::wstring& easeName, EaseFunction ease, int row) {
        const float x = ((float)row - 3.5f) * laneGap;
        const float laneLength = EaseRunnerController::endZ - EaseRunnerController::startZ;

        // 動く範囲を示す細長い線(光の影響を受けない)
        GameObject* track = scene.CreateObject("EaseTrack");
        track->transform.position = { x, trackY, EaseRunnerController::startZ + laneLength * 0.5f };
        track->transform.scale = { 8.f, 4.f, laneLength };
        auto* trackMesh = track->AddComponent<MeshRenderer>(Mesh::CreateCube());
        trackMesh->SetColor(0.35f, 0.4f, 0.55f, 1.0f);
        trackMesh->SetUnlit(true);

        // 動く立方体(色は動き出す時に、コントローラが行ごとに決める)
        GameObject* runner = scene.CreateObject("EaseRunner");
        runner->transform.position = { x, 0.f, EaseRunnerController::startZ };
        runner->transform.scale = { cubeSize, cubeSize, cubeSize };
        runner->AddComponent<MeshRenderer>(Mesh::CreateCube());

        // 動かす役と、その設定をする役(動かす役を先に付ける)
        runner->AddComponent<TweenComponent>();
        runner->AddComponent<EaseRunnerController>(easeName, ease, DemoMode::Mode3D, row);
        return runner;
    }
}

GameObject* CreateEaseRunner(Scene& scene, const std::wstring& easeName, EaseFunction ease, int row, DemoMode mode) {
    if (mode == DemoMode::Mode3D) return CreateEaseRunner3D(scene, easeName, ease, row);
    return CreateEaseRunner2D(scene, easeName, ease, row);
}
