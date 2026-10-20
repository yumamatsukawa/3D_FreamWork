#pragma once
#include "../../../../Engine/Core/Scene.h"

// Sound3D(位置で聞こえ方が変わる音)のデモのシーン(2D版)
class Sound3DDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
