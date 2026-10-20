#pragma once
#include "../../../../Engine/Core/Scene.h"

// Sound3D(位置で聞こえ方が変わる音)のデモのシーン(3D版)
class Sound3DDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
