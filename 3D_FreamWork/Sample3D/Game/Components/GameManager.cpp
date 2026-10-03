#include "GameManager.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/SceneManager.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../Scenes/TitleScene.h"

void GameManager::Init() {
    // Colliderの枠を表示する(デバッグ用。不要ならfalseに)
    RigidbodyComponent::SetDebugDrawEnabled(true);

    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している

    // 敵を倒したらスコア加算
    EventBus::Get().Subscribe("EnemyDefeated", [this]() {
        score += 10;
    });

    // Playerが倒れたらゲームオーバー
    EventBus::Get().Subscribe("PlayerDied", [this]() {
        gameOver = true;
    });
}

void GameManager::Update(float dt) {
    // ゲームオーバー中にSPACEでタイトルへ
    if (gameOver && Input::GetKeyDown(KEY_SPACE)) {
        SceneManager::Get().ChangeScene<TitleScene>();
    }
}
