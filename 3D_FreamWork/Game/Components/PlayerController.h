#pragma once
#include "../../Engine/Component.h"

// プレイヤーの移動・入力操作を担当するComponent。
// 見た目はSpriteRenderer、動きはこちらと役割を分けている。
class PlayerController : public Component {
private:
    float speed = 200.0f;

public:
    void Update(float dt) override;
    void OnCollisionStay2D(CollisionInfo info) override;
};
