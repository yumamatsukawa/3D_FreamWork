#pragma once
#include "../../../../Engine/Core/Scene.h"

// 物理の問い合わせのデモのシーン(3D版)
class QueryDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
