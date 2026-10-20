#pragma once
#include "../../../../Engine/Core/Component.h"

// 付けたオブジェクトを、付けた時の高さを中心に上下にゆらゆら動かす部品
class BobComponent : public Component {
private:
    float height;            // 上下に動く幅(中心から片側)
    float speed;             // ゆれる速さ(ラジアン / 秒)
    float baseY = 0.f;       // ゆれの中心の高さ(Initで今の位置を覚える)
    float elapsed = 0.f;     // 動き始めてからの時間(止めている間は進まない)

public:
    BobComponent(float bobHeight, float bobSpeed) : height(bobHeight), speed(bobSpeed) {}

    void Init() override;
    void Update(float dt) override;
};
