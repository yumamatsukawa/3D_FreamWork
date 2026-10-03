#pragma once
#include "../../../Engine/Component.h"
#include <DirectXMath.h>

class ObjectPool;
class GameObject;

// 敵の動き(地面の上でtargetを追いかける)と、弾に当たった時の処理を担当する
class EnemyController : public Component {
private:
    GameObject* target = nullptr;   // 追いかける相手
    float speed = 60.0f;            // 移動速度
    ObjectPool* pool = nullptr;     // 自分が所属するプール(倒された時に返す)

public:
    // 出現する度に呼ばれ、状態をリセットする
    void Spawn(GameObject* chaseTarget, ObjectPool* ownerPool) {
        target = chaseTarget;
        pool = ownerPool;
    }

    void Update(float dt) override;
    void OnTriggerEnter2D(Collider2D* other) override;
};
