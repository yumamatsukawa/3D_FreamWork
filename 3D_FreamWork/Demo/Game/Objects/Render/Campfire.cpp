#include "Campfire.h"
#include "../../Components/Render/ParticleDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/ParticleRenderer.h"

GameObject* CreateCampfire(Scene& scene, DirectX::XMFLOAT3 position, DemoMode mode) {
    const float scale = GetParticleScale(mode);   // 2Dは1倍、3Dは大きく

    // 煙: 加算合成にしない普通の合成。炎より先に描く(drawPriorityを小さくする)
    ParticleSettings smokeSettings;
    smokeSettings.emitRate = 12.f;
    smokeSettings.lifetimeMin = 1.5f;
    smokeSettings.lifetimeMax = 2.5f;
    smokeSettings.speedMin = 40.f;
    smokeSettings.speedMax = 70.f;
    smokeSettings.direction = { 0.f, 1.f, 0.f };
    smokeSettings.spreadAngle = 30.f;
    smokeSettings.startSize = 30.f;
    smokeSettings.endSize = 100.f;
    smokeSettings.startColor = { 0.5f, 0.5f, 0.5f, 0.7f };
    smokeSettings.endColor = { 0.2f, 0.2f, 0.2f, 0.0f };
    smokeSettings.additive = false;
    GameObject* smoke = scene.CreateObject("CampfireSmoke");
    smoke->transform.position = { position.x, position.y + 30.f * scale, position.z };
    smoke->SetDrawPriority(-1);
    smoke->AddComponent<ParticleRenderer>(AdaptParticleSettings(smokeSettings, mode))->Play();

    // 炎: 上へ昇りながら、黄色からオレンジ・透明へ色が変わって小さくなる(加算合成)
    ParticleSettings flameSettings;
    flameSettings.emitRate = 90.f;
    flameSettings.lifetimeMin = 0.4f;
    flameSettings.lifetimeMax = 0.8f;
    flameSettings.speedMin = 60.f;
    flameSettings.speedMax = 140.f;
    flameSettings.direction = { 0.f, 1.f, 0.f };
    flameSettings.spreadAngle = 30.f;
    flameSettings.startSize = 36.f;
    flameSettings.endSize = 4.f;
    flameSettings.startColor = { 1.0f, 0.9f, 0.3f, 1.0f };
    flameSettings.endColor = { 1.0f, 0.2f, 0.0f, 0.0f };
    flameSettings.additive = true;
    GameObject* flame = scene.CreateObject("Campfire");
    flame->transform.position = position;
    flame->AddComponent<ParticleRenderer>(AdaptParticleSettings(flameSettings, mode))->Play();
    return flame;
}
