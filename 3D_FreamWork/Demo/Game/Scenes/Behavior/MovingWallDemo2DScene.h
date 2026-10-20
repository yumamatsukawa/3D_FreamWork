#pragma once
#include "../../../../Engine/Core/Scene.h"

// 動く壁と経路探索のデモのシーン(2D版)
class MovingWallDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
