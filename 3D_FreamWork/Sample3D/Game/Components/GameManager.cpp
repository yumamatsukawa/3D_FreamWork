#include "GameManager.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/SceneManager.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/SaveData.h"
#include "../Scenes/TitleScene.h"

namespace {
    constexpr const char* highScoreKey = "Sample3DHighScore";   // セーブデータのキー(Sample2Dと同じsave0.txtに書くので、サンプルごとに別の名前にする)
}

void GameManager::Init() {
    // Colliderの枠を表示する(デバッグ用。不要ならfalseに)
    RigidbodyComponent::SetDebugDrawEnabled(true);

    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している

    // 敵を倒したらスコア加算
    EventBus::Get().Subscribe("EnemyDefeated", [this]() {
        score += 10;
    });

    // Playerが倒れたらゲームオーバー。ハイスコアを超えていたら保存する
    EventBus::Get().Subscribe("PlayerDied", [this]() {
        gameOver = true;
        if (score > GetHighScore()) {
            SaveData::SetInt(highScoreKey, score);
            SaveData::Save();
        }
    });
}

int GameManager::GetHighScore() const {
    return SaveData::GetInt(highScoreKey);
}

void GameManager::Update(float dt) {
    // ゲームオーバー中にSPACEでタイトルへ(黒い帯が左から入って覆い、コマ7で切り替えて、右へ抜けていく)
    if (gameOver && Input::GetKeyDown(KEY_SPACE)) {
        SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_wipe.png", 8, 2, 0.04f).SetSwitchFrame(7));
    }
}
