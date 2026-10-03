#pragma once
#include "../../../Engine/Component.h"
#include <DirectXMath.h>

class ObjectPool;
class GameObject;

// 弾の移動と寿命を担当する
class BulletController : public Component {
private:
    DirectX::XMFLOAT3 direction{ 0.f, 0.f, 1.f };   // 進む方向(地面と水平。Yは0)
    float speed = 500.0f;                           // 移動速度
    float lifeTime = 2.0f;                          // 寿命(秒)
    float elapsed = 0.f;                            // 発射されてからの経過時間
    ObjectPool* pool = nullptr;                     // 自分が所属するプール(消える時に返す)

public:
    // 発射される度に呼ばれ、状態をリセットする
    void Fire(DirectX::XMFLOAT3 dir, ObjectPool* ownerPool) {
        direction = dir;
        pool = ownerPool;
        elapsed = 0.f;
    }

    void Update(float dt) override;
    void OnTriggerEnter2D(Collider2D* other) override;
};
