#pragma once
#include "MoveComponent.h"

// 目的地(相手・場所)へ、一定の速さでまっすぐ向かうComponent。StopDistance(既定1.0)より近づいたら止まる。
// 決まりはMoveComponentのコメントを参照。
//
// 使い方:
//   auto* chase = enemy->AddComponent<ChaseComponent>();
//   chase->SetTarget(player);
//   chase->SetSpeed(60.f);
//   chase->SetPlane(MovePlane::XZ);   // 3Dの地面の上を動く時(2Dなら不要)
class ChaseComponent : public MoveComponent {
public:
    void Update(float dt) override;
};
