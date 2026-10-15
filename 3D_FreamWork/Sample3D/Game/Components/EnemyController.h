#pragma once
#include "../../../Engine/Component.h"

class ObjectPool;

// 敵が弾に当たった時の処理を担当する(追いかける動きはChaseComponent、歩きアニメーションはSpriteAnimationComponentが担当)
class EnemyController : public Component {
private:
    ObjectPool* pool = nullptr;     // 自分が所属するプール(倒された時に返す)

public:
    // 出現する度に呼ばれ、返却先を受け取る
    void Spawn(ObjectPool* ownerPool) { pool = ownerPool; }

    void OnTriggerEnter2D(Collider2D* other) override;
};
