#include "GameManager.h"
#include "../../../Engine/Core/EventBus.h"
#include "../../../Engine/Core/Input.h"
#include "../../../Engine/Core/SceneManager.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../Engine/Behavior/VisionComponent.h"
#include "../../../Engine/Data/SaveData.h"
#include "../Scenes/TitleScene.h"

namespace {
    constexpr const char* highScoreKey = "Sample2DHighScore";   // セーブデータのキー(Sample3Dと同じsave0.txtに書くので、サンプルごとに別の名前にする)
}

void GameManager::Init() {
    // Colliderの枠を表示する(デバッグ用。不要ならfalseに)
    RigidbodyComponent::SetDebugDrawEnabled(true);
    VisionComponent::SetDebugDrawEnabled(true);   // 見張り役の視野(扇形)を線で表示する(デバッグ用。不要ならfalseに)

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
    // ゲームオーバー中にSPACEでタイトルへ(黒い丸が閉じていく画像で覆って切り替え、逆再生で開く)
    if (gameOver && Input::GetKeyDown(KEY_SPACE)) {
        SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_circle.png", 8, 2, 0.04f));
    }
}
