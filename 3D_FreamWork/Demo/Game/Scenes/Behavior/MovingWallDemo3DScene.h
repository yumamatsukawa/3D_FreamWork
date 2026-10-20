#pragma once
#include "../../../../Engine/Core/Scene.h"

// 動く壁と経路探索のデモのシーン(3D版)
class MovingWallDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
