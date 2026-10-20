#include "DoorController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/MathUtil.h"

void DoorController::Setup(DirectX::XMFLOAT3 closed, DirectX::XMFLOAT3 opened, float moveSpeed) {
    closedPosition = closed;
    openPosition = opened;
    speed = moveSpeed;
    GetOwner()->transform.position = closedPosition;
}

void DoorController::Update(float dt) {
    // 開ける指示なら開いた位置へ、閉める指示なら閉じた位置へ、一定の速さで動く
    const DirectX::XMFLOAT3 goal = open ? openPosition : closedPosition;
    Transform& transform = GetOwner()->transform;
    moving = Math::DistanceSq(transform.position, goal) > 0.0001f;
    if (moving) transform.position = Math::MoveTowards(transform.position, goal, speed * dt);
}
