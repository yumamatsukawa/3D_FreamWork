#pragma once
#include "../../../Engine/Component.h"

// プレイヤーの移動・入力操作を担当するComponent。
// 見た目はSpriteRenderer、動きはこちらと役割を分けている。
class PlayerController : public Component {
private:
    float speed = 200.0f;
    // ★ OnCollisionStay2Dは、Update()より前(Physics::Updateの中)で毎フレーム呼ばれる。
    //   Update()側で毎フレーム無条件に白へ戻すと、そのフレームの赤色がすぐ上書きされて
    //   消えてしまうため、このフラグで「今フレームEnemyと接触していたか」を覚えておく
    bool touchingEnemy = false;

public:
    void Update(float dt) override;
    void OnCollisionStay2D(CollisionInfo info) override;
};
