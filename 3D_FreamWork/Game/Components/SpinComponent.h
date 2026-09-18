#pragma once
#include "../../Engine/Component.h"
#include "../../Engine/GameObject.h"

// デモ・確認用: 毎フレーム一定角速度で回転し続けるComponent。
// 3Dメッシュがちゃんと立体として回っているか目視確認するために使う
class SpinComponent : public Component {
private:
    DirectX::XMFLOAT3 speedDegPerSec{ 30.f, 45.f, 0.f };

public:
    explicit SpinComponent(DirectX::XMFLOAT3 speed = { 30.f, 45.f, 0.f }) : speedDegPerSec(speed) {}

    void Update(float dt) override {
        Transform& t = GetOwner()->transform;
        t.rotate.x += speedDegPerSec.x * dt;
        t.rotate.y += speedDegPerSec.y * dt;
        t.rotate.z += speedDegPerSec.z * dt;
    }
};
