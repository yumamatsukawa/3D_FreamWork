#pragma once
#include "../../../../Engine/Core/Scene.h"

// 物理の問い合わせのデモのシーン(2D版)
class QueryDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
