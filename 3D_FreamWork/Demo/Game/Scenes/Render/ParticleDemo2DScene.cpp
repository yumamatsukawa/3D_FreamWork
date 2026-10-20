#include "ParticleDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/Campfire.h"
#include "../../Objects/Render/ParticleDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/ParticleRenderer.h"

void ParticleDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"パーティクルデモ(ParticleRenderer / CreateEffect)",
        L"小さい粒をたくさん出して、爆発・火花・煙などを作ります。下の焚き火は出しっぱなしです。",
        L"操作: 1〜4 で種類を選ぶ / 画面をクリックで発射 / SPACE を押している間はスロー / Tab: 3D版へ / Esc: 戻る",
        L"使っているAPI: ParticleSettings(additive, gravity, startColor/endColor), CreateEffect, Burst, Play" });
    GameObject* campfire = CreateCampfire(*this, { 0.0f, -230.0f, 0.0f }, DemoMode::Mode2D);
    CreateParticleDemo(*this, campfire->GetComponent<ParticleRenderer>(), DemoMode::Mode2D);
}

void ParticleDemo2DScene::Uninit()
{
    Scene::Uninit();
}
