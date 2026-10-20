#pragma once
#include "../../../../Engine/Core/Scene.h"

// 視野のデモのシーン(3D版)
class VisionDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
