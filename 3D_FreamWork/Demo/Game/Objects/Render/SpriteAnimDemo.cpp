#include "SpriteAnimDemo.h"
#include "../../Components/Render/SpriteAnimDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Animation/SpriteAnimationComponent.h"

namespace {
    // 上の段のクリップ1つ分(player.png は 8列x2行。0〜7が上の段、8〜15が下の段)
    struct ClipSpec {
        const char* name;
        std::vector<int> frames;
        float frameDuration;
        bool loop;
        const wchar_t* label;
    };
    const ClipSpec clipSpecs[4] = {
        { "Walk",   { 0, 1 },                   0.25f, true,  L"Walk(ループ)" },
        { "Scared", { 8, 9 },                   0.2f,  true,  L"Scared(ループ)" },
        { "Eyes",   { 12, 13, 14, 15 },         0.3f,  false, L"Eyes(1回だけ)" },
        { "All",    { 0, 1, 2, 3, 4, 5, 6, 7 }, 0.15f, true,  L"All(8コマ)" },
    };
}

GameObject* CreateSpriteAnimDemo(Scene& scene, DemoMode mode) {
    const bool is3D = (mode == DemoMode::Mode3D);

    // 操作役(スプライトを足していく)
    GameObject* demo = scene.CreateObject("SpriteAnimDemo");
    SpriteAnimDemoController* controller = demo->AddComponent<SpriteAnimDemoController>(mode);

    // スプライトを1つ作る(画像 → ビルボード設定 → アニメーションの順に付ける。アニメーションはSpriteRendererの後)
    auto createGhost = [&](const DirectX::XMFLOAT3& position, float size) {
        GameObject* object = scene.CreateObject("Ghost");
        object->transform.position = position;
        object->transform.scale = { size, size, 1.f };
        SpriteRenderer* sprite = object->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
        if (is3D) {
            sprite->SetWorldSpace(true);
            sprite->SetBillboardLock(true, false, true);   // 水平方向だけカメラへ向く(立て看板)
        }
        return object;
    };

    // 上の段: クリップの違う4つ(始まりはそろえる)
    for (int i = 0; i < 4; i++) {
        const ClipSpec& spec = clipSpecs[i];
        const DirectX::XMFLOAT3 position = is3D ? DirectX::XMFLOAT3{ -300.f + 200.f * (float)i, 0.f, 300.f }
                                                : DirectX::XMFLOAT3{ -420.f + 280.f * (float)i, 40.f, 0.f };
        GameObject* ghost = createGhost(position, is3D ? 100.f : 110.f);
        auto* anim = ghost->AddComponent<SpriteAnimationComponent>();
        anim->AddClip(spec.name, spec.frames, spec.frameDuration, spec.loop);
        anim->Play(spec.name);
        controller->AddEntry(ghost, anim, spec.name, !spec.loop, spec.label, is3D ? 85.f : -80.f);
    }

    // 下の段: 同じクリップで、始まるコマをずらした5つ(SetRandomStart。上の Walk と見比べる)
    for (int i = 0; i < 5; i++) {
        const DirectX::XMFLOAT3 position = is3D ? DirectX::XMFLOAT3{ -240.f + 120.f * (float)i, -15.f, 520.f }
                                                : DirectX::XMFLOAT3{ -300.f + 150.f * (float)i, -150.f, 0.f };
        GameObject* ghost = createGhost(position, 70.f);
        auto* anim = ghost->AddComponent<SpriteAnimationComponent>();
        anim->AddClip("Walk", { 0, 1 }, 0.25f, true);
        anim->SetRandomStart(true);
        anim->Play("Walk");
        controller->AddEntry(ghost, anim, "Walk", false, (i == 2) ? L"SetRandomStart(true)" : L"", is3D ? 65.f : -50.f);
    }
    return demo;
}
