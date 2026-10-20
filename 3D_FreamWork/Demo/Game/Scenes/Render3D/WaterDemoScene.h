#pragma once
#include "../../../../Engine/Core/Scene.h"

// 水面(WaterRenderer)のデモのシーン(3Dだけ)
class WaterDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
