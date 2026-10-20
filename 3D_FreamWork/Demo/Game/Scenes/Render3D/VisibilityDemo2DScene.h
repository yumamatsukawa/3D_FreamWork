#pragma once
#include "../../../../Engine/Core/Scene.h"

// 画面に映っているかの判定のデモのシーン(2D版)
class VisibilityDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
