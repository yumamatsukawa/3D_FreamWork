#pragma once
#include "../../../Engine/Component.h"
#include <DirectXMath.h>

class ObjectPool;
class GameObject;

// Playerへゆっくり近づき続け、弾(タグ"Bullet")に当たったら倒される敵。
// プーリングで使い回すため、状態のリセットはInit()ではなくSpawn()で行う
// (Init()はAddComponentされた最初の1回しか呼ばれないため)
class EnemyController : public Component {
private:
    GameObject* target = nullptr;   // 追いかける相手(Player)
    float speed = 60.0f;
    ObjectPool* pool = nullptr;

public:
    // 出現する度に呼ばれ、状態をリセットする
    void Spawn(GameObject* chaseTarget, ObjectPool* ownerPool) {
        target = chaseTarget;
        pool = ownerPool;
    }

    void Update(float dt) override;
    void OnTriggerEnter2D(Collider2D* other) override;
};
