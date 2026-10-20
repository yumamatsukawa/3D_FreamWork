#pragma once
#include "../../../../Engine/Core/Scene.h"

// 当たり判定の種類のデモのシーン(2D版)
class ColliderDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
