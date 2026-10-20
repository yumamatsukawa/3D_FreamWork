#include "WorldUiDemo.h"
#include "../../Components/UI/WorldUiDemoController.h"
#include "../../Components/Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/UI/GaugeRenderer.h"
#include "../../../../Engine/UI/TextRenderer.h"

namespace {
    // 敵の見た目の設定(名前・最大HP・位置・動く先・色)
    struct EnemySpec {
        const wchar_t* name;
        float maxHp;
        DirectX::XMFLOAT3 position2D;
        DirectX::XMFLOAT3 moveTo2D;      // 動き続ける先(今の位置と同じなら動かない)
        DirectX::XMFLOAT3 position3D;
        DirectX::XMFLOAT3 moveTo3D;
        DirectX::XMFLOAT4 color;         // 3D版の色
        int spriteIndex;                 // 2D版のコマ
    };
    const EnemySpec enemySpecs[3] = {
        { L"Slime",  60.f,  { -350.f, 30.f, 0.f }, { -350.f, 30.f, 0.f }, { -300.f, -10.f, 400.f }, { -300.f, -10.f, 400.f }, { 0.4f, 0.9f, 0.4f, 1.0f }, 1 },
        { L"Goblin", 80.f,  { -150.f, -40.f, 0.f }, { 150.f, -40.f, 0.f }, { -150.f, -10.f, 300.f }, { 150.f, -10.f, 300.f }, { 0.9f, 0.5f, 0.3f, 1.0f }, 2 },
        { L"Golem",  120.f, { 350.f, 30.f, 0.f },   { 350.f, -90.f, 0.f }, { 300.f, -10.f, 300.f },  { 300.f, -10.f, 480.f }, { 0.5f, 0.6f, 0.9f, 1.0f }, 3 },
    };
}

GameObject* CreateWorldUiDemo(Scene& scene, DemoMode mode) {
    const bool is3D = (mode == DemoMode::Mode3D);

    // 主人公(矢印キーで動く。頭の上の文字は TextRenderer で、物に付いて動く)
    GameObject* hero = scene.CreateObject("Hero");
    if (is3D) {
        hero->transform.position = { 0.f, -25.f, 150.f };
        hero->transform.scale = { 50.f, 50.f, 50.f };
        hero->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(1.0f, 0.9f, 0.3f, 1.0f);
    }
    else {
        hero->transform.position = { 0.f, -150.f, 0.f };
        hero->transform.scale = { 56.f, 56.f, 1.f };
        hero->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(5);
    }
    TextRenderer* heroText = hero->AddComponent<TextRenderer>();
    heroText->SetText(L"Hero");
    heroText->SetStyle(MakeDemoTextStyle(20.f, { 1.0f, 0.9f, 0.4f, 1.0f }, TextAlign::Center));
    heroText->SetOffset({ 0.f, 50.f, 0.f });
    heroText->SetUse3DCamera(is3D);

    // 操作役(敵のHPの持ち主)
    GameObject* demo = scene.CreateObject("WorldUiDemo");
    WorldUiDemoController* controller = demo->AddComponent<WorldUiDemoController>(mode, hero);

    // 敵3体: 動く敵もいる。頭上のHPゲージは、敵に付けて「敵を追う」設定にする(ゲージは値を読むだけ)
    for (const EnemySpec& spec : enemySpecs) {
        GameObject* enemy = scene.CreateObject("Enemy");
        const DirectX::XMFLOAT3 position = is3D ? spec.position3D : spec.position2D;
        const DirectX::XMFLOAT3 moveTo = is3D ? spec.moveTo3D : spec.moveTo2D;
        enemy->transform.position = position;
        if (is3D) {
            enemy->transform.scale = { 70.f, 70.f, 70.f };
            enemy->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(spec.color.x, spec.color.y, spec.color.z, spec.color.w);
        }
        else {
            enemy->transform.scale = { 70.f, 70.f, 1.f };
            enemy->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(spec.spriteIndex);
        }
        if (moveTo.x != position.x || moveTo.y != position.y || moveTo.z != position.z) {
            enemy->AddComponent<TweenComponent>()->MoveTo(moveTo, 2.5f, Ease::InOutQuad).SetLoop(true, true);
        }

        const int index = controller->AddEnemy(enemy, spec.name, spec.maxHp);
        GaugeRenderer* gauge = enemy->AddComponent<GaugeRenderer>();
        gauge->SetSize(90.f, 10.f);
        gauge->SetColors({ 1.0f, 0.4f, 0.4f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.6f });
        gauge->SetDelayedBar(true, { 1.0f, 0.9f, 0.4f, 1.0f });
        gauge->SetFollowTarget(enemy, { 0.f, 70.f });
        gauge->SetUse3DCamera(is3D);
        gauge->SetValueSource([controller, index] { return controller->GetEnemyHp(index); }, [controller, index] { return controller->GetEnemyMaxHp(index); });
    }
    return demo;
}
