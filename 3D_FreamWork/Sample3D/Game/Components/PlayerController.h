#pragma once
#include "../../../Engine/Component.h"

class CameraController;

// Playerの操作(カメラ基準の移動・カメラと同じ向きを向く・発射)とHPを担当する
class PlayerController : public Component {
private:
    CameraController* camera = nullptr;   // 向き・移動の基準にするカメラ
    float speed = 200.0f;                 // 移動速度
    bool touchingEnemy = false;           // このフレームに敵と接触しているか(色を戻さないため)
    float colliderRotateY = 0.f;          // 当たり判定の箱(PhysX)に反映済みのrotate.y(度)

    int hp = 3;                         // 現在のHP
    static constexpr int maxHp = 3;     // 最大HP

    float invincibleTimer = 0.f;                        // 無敵時間の残り
    static constexpr float invincibleDuration = 1.0f;   // ダメージを受けた後の無敵時間(秒)

public:
    // 向き・移動の基準にするカメラを、CreatePlayer()から渡してもらう
    void Setup(CameraController* cameraController) { camera = cameraController; }

    void Update(float dt) override;
    void OnCollisionStay2D(CollisionInfo info) override;

    // HUDなどが表示に使う
    int GetHp() const { return hp; }
    int GetMaxHp() const { return maxHp; }
};
