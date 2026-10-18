#include "LanternController.h"
#include "../../../Engine/Core/GameObject.h"
#include <cmath>

void LanternController::Update(float dt) {
    // 移動処理: 中心の周りを回りつつ、sinで上下に揺れる
    angle += orbitSpeed * dt;
    bobTimer += dt;
    float rad = DirectX::XMConvertToRadians(angle);

    Transform& t = GetOwner()->transform;
    t.position.x = center.x + cosf(rad) * orbitRadius;
    t.position.z = center.z + sinf(rad) * orbitRadius;
    t.position.y = center.y + baseHeight + sinf(bobTimer * bobSpeed) * bobHeight;
}
