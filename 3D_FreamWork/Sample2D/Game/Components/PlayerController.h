#pragma once
#include "../../../Engine/Component.h"

// Playerの操作(移動・向き・発射)とHPを担当する
class PlayerController : public Component {
private:
    float speed = 200.0f;          // 移動速度
    bool touchingEnemy = false;    // このフレームに敵と接触しているか(色を戻さないため)

    int hp = 3;                         // 現在のHP
    static constexpr int maxHp = 3;     // 最大HP

    float invincibleTimer = 0.f;                        // 無敵時間の残り
    static constexpr float invincibleDuration = 1.0f;   // ダメージを受けた後の無敵時間(秒)

public:
    void Update(float dt) override;
    void OnCollisionStay2D(CollisionInfo info) override;

    // HUDなどが表示に使う
    int GetHp() const { return hp; }
    int GetMaxHp() const { return maxHp; }
};
