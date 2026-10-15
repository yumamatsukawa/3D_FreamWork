#pragma once
#include "../../../Engine/Component.h"
#include "../../../Engine/Timer.h"

// Playerの操作(移動・向き・発射)とHPを担当する
class PlayerController : public Component {
private:
    float speed = 200.0f;          // 移動速度
    bool touchingEnemy = false;    // このフレームに敵と接触しているか(色を戻さないため)

    int hp = 3;                         // 現在のHP
    static constexpr int maxHp = 3;     // 最大HP

    static constexpr float invincibleDuration = 1.0f;   // ダメージを受けた後の無敵時間(秒)
    Timer invincibleTimer{ invincibleDuration };         // 無敵時間を数える(終わっていれば無敵ではない)

    static constexpr float knockbackSpeed = 800.0f;     // 敵に当たった時に弾かれる速さ
    static constexpr float knockbackDuration = 0.2f;    // 弾かれている時間(秒。この間は移動キーが効かない)
    Timer knockbackTimer{ knockbackDuration };           // 弾かれている時間を数える(終わっていれば弾かれていない)

    static constexpr float damageShakeStrength = 6.0f;  // ダメージを受けた時の画面の揺れの強さ(ピクセル)
    static constexpr float damageShakeDuration = 0.2f;  // ダメージを受けた時に画面が揺れる時間(秒)

    static constexpr DirectX::XMFLOAT4 damageVignetteColor = { 1.0f, 0.0f, 0.0f, 0.6f };  // ダメージを受けた時に画面の端に出す色(wは一番濃い時の濃さ)
    static constexpr float damageVignetteDuration = 0.5f;   // 画面の端が点滅する時間(秒)
    static constexpr int damageVignetteBlinkCount = 2;      // 画面の端が点滅する回数

public:
    void Init() override;
    void Update(float dt) override;
    void OnCollisionStay2D(CollisionInfo info) override;

    // HUDなどが表示に使う
    int GetHp() const { return hp; }
    int GetMaxHp() const { return maxHp; }
};
