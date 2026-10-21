#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class WaterRenderer;
class RigidbodyComponent;

// 物理(Dynamic)の球を、水に浮かべる(沈んだ深さに応じた上向きの力=浮力と、水の抵抗を毎フレーム加える)。
// 球の真下の波の高さは WaterRenderer::GetHeightAt で調べる。水位や波が変わっても追従する
class BuoyancyController : public Component {
private:
    WaterRenderer* water = nullptr;            // 浮かぶ水面
    float radius = 0.f;                        // 球の半径
    DirectX::XMFLOAT3 startPosition{};         // 最初の位置(遠くへ落ちたら、ここへ戻す)
    RigidbodyComponent* rigidbody = nullptr;   // 同じオブジェクトの剛体(力を加える)

public:
    BuoyancyController(WaterRenderer* waterSurface, float sphereRadius, DirectX::XMFLOAT3 start)
        : water(waterSurface), radius(sphereRadius), startPosition(start) {}

    void Update(float dt) override;
};
