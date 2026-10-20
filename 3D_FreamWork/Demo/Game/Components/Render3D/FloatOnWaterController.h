#pragma once
#include "../../../../Engine/Core/Component.h"

class WaterRenderer;

// 水面に浮かぶ(毎フレーム、波の高さに合わせて、自分の高さを変える。WaterRenderer::GetHeightAt を使う)
class FloatOnWaterController : public Component {
private:
    WaterRenderer* water = nullptr;   // 浮かぶ水面
    float heightOffset = 0.f;         // 波の高さから、中心をずらす量(物の半分の高さ)

public:
    FloatOnWaterController(WaterRenderer* waterSurface, float offset) : water(waterSurface), heightOffset(offset) {}

    void Update(float dt) override;
};
