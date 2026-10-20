#include "SpinComponent.h"
#include "../../../../Engine/Core/GameObject.h"

void SpinComponent::Update(float dt) {
    // 回転: 2DはZ軸、3DはY軸(Transformの rotate は度)
    Transform& t = GetOwner()->transform;
    if (mode == DemoMode::Mode3D) t.rotate.y += degreesPerSecond * dt;
    else t.rotate.z += degreesPerSecond * dt;
}
