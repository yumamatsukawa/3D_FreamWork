#include "GameManager.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/SceneManager.h"
#include "../Scenes/TitleScene.h"
#include <DirectXMath.h>
#include <cstdlib>

void GameManager::Init() {
    // ★ ラムダの中でthis(GameManager自身)をキャプチャしている。GameSceneのUninit()が
    //   EventBus::Get().Clear()を呼ぶので、シーンが終わる時に登録も消える(GameScene.cpp参照)
    EventBus::Get().Subscribe("EnemyDefeated", [this]() {
        score += 10;
    });
    EventBus::Get().Subscribe("PlayerDied", [this]() {
        gameOver = true;
    });
}

void GameManager::Update(float dt) {
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
