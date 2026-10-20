#pragma once
#include "../../../../Engine/Core/Scene.h"

// 追いかけ方の比較のデモのシーン(3D版)
class HomingDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
