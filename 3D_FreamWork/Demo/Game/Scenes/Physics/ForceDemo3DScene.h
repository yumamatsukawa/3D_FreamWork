#pragma once
#include "../../../../Engine/Core/Scene.h"

// 力と速度のデモのシーン(3D版)
class ForceDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
