#pragma once
#include "../../../Engine/Component.h"

// スコアとゲームオーバーを管理する(敵の出現はEnemyManager、弾の発射はBulletManagerの担当)
class GameManager : public Component {
private:
    int score = 0;            // スコア
    bool gameOver = false;    // ゲームオーバーになったか

public:
    void Init() override;
    void Update(float dt) override;

    // HUDなどが表示に使う
    int GetScore() const { return score; }
    bool IsGameOver() const { return gameOver; }
};
