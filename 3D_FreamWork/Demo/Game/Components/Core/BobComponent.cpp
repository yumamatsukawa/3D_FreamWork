#include "BobComponent.h"
#include "../../../../Engine/Core/GameObject.h"
#include <cmath>

void BobComponent::Init() {
    // 付けた時の高さを、ゆれの中心にする
    baseY = GetOwner()->transform.position.y;
}

void BobComponent::Update(float dt) {
    // 上下の動き(sinで-1〜1を往復する)
    elapsed += dt;
    GetOwner()->transform.position.y = baseY + sinf(elapsed * speed) * height;
}
