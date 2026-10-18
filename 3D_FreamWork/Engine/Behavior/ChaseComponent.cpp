#include "ChaseComponent.h"
#include "../Core/MathUtil.h"

void ChaseComponent::Update(float dt) {
    // 目的地が無い・着いた時は止まる(ほぼ重なっている時は方向が定まらないため)
    XMFLOAT3 offset;
    if (!GetOffsetToDestination(offset) || IsWithinStopDistance(offset)) {
        ApplyVelocity({ 0.f, 0.f, 0.f }, dt);
        return;
    }

    // 移動処理: 目的地の方へ一定の速さで。1フレームで残りの距離より先へは進まない(行き過ぎて往復しないように)
    float distance = Math::Length(offset);
    float moveSpeed = speed;
    if (dt > 0.f && moveSpeed * dt > distance) moveSpeed = distance / dt;
    ApplyVelocity(Math::Normalize(offset) * moveSpeed, dt);
}
