#pragma once
#include "../../../Engine/Component.h"
#include "../Objects/EnemyManager.h"

class GameObject;

// スコア・HP・敵の出現タイミング・ゲームオーバーを管理する、ゲーム全体の進行役。
// GameScene::Init()で専用のGameObjectを1つ作り、これをAddComponentして使う
// (実体の無いGameObjectでも、Componentならばこうして「ゲーム全体を管理する係」になれる)
class GameManager : public Component {
private:
    EnemyManager* enemyManager = nullptr;
    GameObject* player = nullptr;

    int score = 0;
    int hp = 3;
    static constexpr int maxHp = 3;

    float invincibleTimer = 0.f;
    static constexpr float invincibleDuration = 1.0f;  // Enemyに当たってから、次に減点されるまでの猶予

    float spawnTimer = 1.0f;   // 開始直後にいきなり湧かないよう、最初だけ少し待つ
    static constexpr float spawnInterval = 2.5f;
    static constexpr float spawnDistance = 600.0f;  // Playerからどれだけ離れた場所に出現させるか

    bool gameOver = false;

public:
    // EnemyManager/Playerへの参照を、GameScene::Init()側から渡してもらう
    void Setup(EnemyManager* manager, GameObject* playerObject) {
        enemyManager = manager;
        player = playerObject;
    }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
