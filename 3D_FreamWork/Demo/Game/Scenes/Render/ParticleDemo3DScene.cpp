#include "ParticleDemo3DScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGround.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Render/Campfire.h"
#include "../../Objects/Render/ParticleDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/ParticleRenderer.h"

void ParticleDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoSkybox(*this);
    CreateDemoGround(*this);
    CreateDemoGuide(*this, { L"パーティクルデモ(ParticleRenderer / CreateEffect)",
        L"同じ設定の粒を3D空間に出します(use3D)。粒はカメラの方を向く板で、手前の物に隠れます。中央は出しっぱなしの焚き火です。",
        L"操作: 1〜4 種類 / 地面をクリックで発射 / SPACE スロー / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ / Esc: 戻る",
        L"使っているAPI: ParticleSettings(use3D, additive, gravity, startColor/endColor), CreateEffect, Burst, Play" });
    GameObject* campfire = CreateCampfire(*this, { 0.0f, -40.0f, 300.0f }, DemoMode::Mode3D);
    CreateParticleDemo(*this, campfire->GetComponent<ParticleRenderer>(), DemoMode::Mode3D);
    CreateDemoCamera(*this, { 0.0f, 0.0f, 300.0f }, 0.0f, -25.0f, 900.0f);
}

void ParticleDemo3DScene::Uninit()
{
    Scene::Uninit();
}
