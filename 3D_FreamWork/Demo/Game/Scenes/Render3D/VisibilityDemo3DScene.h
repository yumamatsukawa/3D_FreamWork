#pragma once
#include "../../../../Engine/Core/Scene.h"

// 画面に映っているかの判定のデモのシーン(3D版)
class VisibilityDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
