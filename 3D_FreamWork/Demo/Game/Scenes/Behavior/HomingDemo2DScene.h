#pragma once
#include "../../../../Engine/Core/Scene.h"

// 追いかけ方の比較のデモのシーン(2D版)
class HomingDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
