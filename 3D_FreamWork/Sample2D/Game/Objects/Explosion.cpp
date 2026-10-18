#include "Explosion.h"
#include "../../../Engine/Render/Effect.h"
#include "../../../Engine/Core/GameObject.h"

namespace {
    constexpr int particleCount = 30;   // 一度に出す粒の数
}

GameObject* CreateExplosion(Scene& scene, DirectX::XMFLOAT3 position) {
    // 見た目: 全方向に飛び散る、オレンジ色の光の粒(だんだん小さく・暗くなる)
    ParticleSettings settings;
    settings.lifetimeMin = 0.3f;
    settings.lifetimeMax = 0.7f;
    settings.speedMin = 100.f;
    settings.speedMax = 300.f;
    settings.spreadAngle = 360.f;
    settings.startSize = 50.f;
    settings.endSize = 0.f;
    settings.startColor = { 1.0f, 0.7f, 0.2f, 1.0f };
    settings.endColor = { 1.0f, 0.2f, 0.0f, 0.0f };
    settings.additive = true;

    // 作ったらすぐ粒を出す(出し終わったらEffectが自動で消える)
    GameObject* effect = CreateEffect(scene, settings, position);
    effect->GetComponent<ParticleRenderer>()->Burst(particleCount);
    return effect;
}
