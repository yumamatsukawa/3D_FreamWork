#pragma once
#include "../../../../Engine/Core/Scene.h"

// 影(Shadow)のデモのシーン(3Dだけ)
class ShadowDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
