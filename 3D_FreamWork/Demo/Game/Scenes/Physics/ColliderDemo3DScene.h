#pragma once
#include "../../../../Engine/Core/Scene.h"

// 当たり判定の種類のデモのシーン(3D版)
class ColliderDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
