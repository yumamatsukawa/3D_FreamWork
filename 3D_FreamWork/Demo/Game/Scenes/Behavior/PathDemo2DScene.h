#pragma once
#include "../../../../Engine/Core/Scene.h"

// 経路探索のデモのシーン(2D版)
class PathDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
