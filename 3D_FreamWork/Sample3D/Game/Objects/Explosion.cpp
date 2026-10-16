#include "Explosion.h"
#include "../../../Engine/Effect.h"
#include "../../../Engine/GameObject.h"

namespace {
    constexpr int particleCount = 40;   // 一度に出す粒の数
}

GameObject* CreateExplosion(Scene& scene, DirectX::XMFLOAT3 position) {
    // 見た目: 3D空間に全方向へ飛び散る、オレンジ色の光の粒(少し落ちながら、だんだん小さく・暗くなる)
    // 長さの単位はワールド(敵の大きさが100)
    ParticleSettings settings;
    settings.lifetimeMin = 0.4f;
    settings.lifetimeMax = 0.9f;
    settings.speedMin = 200.f;
    settings.speedMax = 600.f;
    settings.spreadAngle = 360.f;
    settings.gravity = { 0.f, -400.f, 0.f };
    settings.startSize = 80.f;
    settings.endSize = 0.f;
    settings.startColor = { 1.0f, 0.7f, 0.2f, 1.0f };
    settings.endColor = { 1.0f, 0.2f, 0.0f, 0.0f };
    settings.additive = true;
    settings.use3D = true;

    // 作ったらすぐ粒を出す(出し終わったらEffectが自動で消える)
    GameObject* effect = CreateEffect(scene, settings, position);
    effect->GetComponent<ParticleRenderer>()->Burst(particleCount);
    return effect;
}
