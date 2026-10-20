#pragma once
#include "../../../../Engine/Core/Scene.h"

// 図形とマテリアルのデモのシーン(3Dだけ)
class ShapesDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
