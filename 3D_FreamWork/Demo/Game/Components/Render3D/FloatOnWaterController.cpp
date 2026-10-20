#include "FloatOnWaterController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/WaterRenderer.h"

void FloatOnWaterController::Update(float dt) {
    if (!water) return;

    // 今の位置(XZ)の波の高さに合わせて、高さを決める(波が上下するのに合わせて、浮かんで揺れる)
    Transform& t = GetOwner()->transform;
    t.position.y = water->GetHeightAt(t.position.x, t.position.z) + heightOffset;
}
