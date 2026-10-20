#pragma once
#include "../../../../Engine/Core/Scene.h"

// 力と速度のデモのシーン(2D版)
class ForceDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
