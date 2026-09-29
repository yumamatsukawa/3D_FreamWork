#include "GameManager.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/SceneManager.h"
#include "../Scenes/TitleScene.h"
#include <DirectXMath.h>
#include <cstdlib>
#include <cstdio>

void GameManager::Init() {
    // ★ ラムダの中でthis(GameManager自身)をキャプチャしている。GameSceneのUninit()が
    //   EventBus::Get().Clear()を呼ぶので、シーンが終わる時に登録も消える(GameScene.cpp参照)
    EventBus::Get().Subscribe("EnemyDefeated", [this]() {
        score += 10;
    });
    EventBus::Get().Subscribe("PlayerHit", [this]() {
        if (gameOver || invincibleTimer > 0.f) return;
        hp--;
        invincibleTimer = invincibleDuration;
        if (hp <= 0) gameOver = true;
    });
}

void GameManager::Update(float dt) {
    if (invincibleTimer > 0.f) invincibleTimer -= dt;

    if (gameOver) {
        if (Input::GetKeyDown(KEY_SPACE)) {
            SceneManager::Get().ChangeScene<TitleScene>();
        }
        return;
    }

    if (!enemyManager || !player) return;

    spawnTimer -= dt;
    if (spawnTimer <= 0.f) {
        spawnTimer = spawnInterval;

        // Playerを中心に、ランダムな方向・一定距離離れた場所に出現させる
        float angle = ((float)rand() / RAND_MAX) * DirectX::XM_2PI;
        DirectX::XMFLOAT3 pos = {
            player->transform.position.x + cosf(angle) * spawnDistance,
            player->transform.position.y + sinf(angle) * spawnDistance,
            0.0f
        };
        enemyManager->Spawn(*GetOwner()->GetScene(), pos, player);
    }
}

void GameManager::Draw() {
    wchar_t buf[64];

    swprintf_s(buf, L"Score: %d", score);
    Text::Draw(buf, -560.f, 320.f, 28.f);

    swprintf_s(buf, L"HP: %d / %d", hp, maxHp);
    Text::Draw(buf, -560.f, 280.f, 28.f, { 1.0f, 0.4f, 0.4f, 1.0f });

    if (gameOver) {
        Text::Draw(L"GAME OVER", 0.f, 40.f, 64.f, { 1.0f, 0.2f, 0.2f, 1.0f });
        Text::Draw(L"press SPACE to return to Title", 0.f, -30.f, 24.f);
    }
}
