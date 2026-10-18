#include "Effect.h"
#include "../Core/Scene.h"
#include "../Core/GameObject.h"

namespace {
    constexpr int effectDrawPriority = 100;   // 普通のオブジェクト(0)より後=手前に描く
}

GameObject* CreateEffect(Scene& scene, const ParticleSettings& settings, DirectX::XMFLOAT3 position) {
    GameObject* effect = scene.CreateObject("Effect");
    effect->transform.position = position;
    effect->SetDrawPriority(effectDrawPriority);

    // 粒がすべて消えたら自動で消える
    auto* particles = effect->AddComponent<ParticleRenderer>(settings);
    particles->SetDestroyWhenFinished(true);
    return effect;
}
