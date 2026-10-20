#pragma once
#include "../../../../Engine/Core/Scene.h"

// 経路探索のデモのシーン(3D版)
class PathDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
