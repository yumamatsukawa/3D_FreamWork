#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"

// 付けたオブジェクトをその場でくるくる回す部品(2D: Z軸まわり / 3D: Y軸まわり)
class SpinComponent : public Component {
private:
    DemoMode mode;               // 2D版か3D版か(回す軸が変わる)
    float degreesPerSecond;      // 1秒あたりに回る角度(度)

public:
    SpinComponent(DemoMode spinMode, float speed) : mode(spinMode), degreesPerSecond(speed) {}

    void Update(float dt) override;
};
