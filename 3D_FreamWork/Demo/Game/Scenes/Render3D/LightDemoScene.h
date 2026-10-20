#pragma once
#include "../../../../Engine/Core/Scene.h"

// マテリアルとライトのデモのシーン(3Dだけ)
class LightDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
