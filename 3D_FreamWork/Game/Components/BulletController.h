#pragma once
#include "../../Engine/Component.h"
#include <DirectXMath.h>

class ObjectPool;
class GameObject;

// 一定方向へ飛び続け、寿命が来た、またはEnemyに当たったらプールに戻る弾。
// プーリングで使い回すため、状態のリセットはInit()ではなくFire()で行う
// (Init()はAddComponentされた最初の1回しか呼ばれないため)
class BulletController : public Component {
private:
    DirectX::XMFLOAT2 direction{ 0.f, -1.f };
    float speed = 500.0f;
    float lifeTime = 2.0f;   // 秒。これを過ぎたら自動でプールに返す
    float elapsed = 0.f;
    ObjectPool* pool = nullptr;

public:
    // 発射される度に呼ばれ、状態をリセットする
    void Fire(DirectX::XMFLOAT2 dir, ObjectPool* ownerPool) {
        direction = dir;
        pool = ownerPool;
        elapsed = 0.f;
    }

    void Update(float dt) override;
    void OnTriggerEnter2D(Collider2D* other) override;
};
