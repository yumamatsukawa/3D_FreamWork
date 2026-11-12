#pragma once
#include "../../../../Engine/Core/Scene.h"

// 負荷テスト(ロボットの大群)のシーン(3Dだけ)
class RobotCrowdScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
