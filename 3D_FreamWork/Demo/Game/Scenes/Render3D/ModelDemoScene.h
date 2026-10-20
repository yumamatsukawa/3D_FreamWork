#pragma once
#include "../../../../Engine/Core/Scene.h"

// FBXモデルのデモのシーン(3Dだけ)
class ModelDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
