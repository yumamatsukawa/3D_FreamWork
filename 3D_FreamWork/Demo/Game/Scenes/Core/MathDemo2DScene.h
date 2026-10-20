#pragma once
#include "../../../../Engine/Core/Scene.h"

// MathUtil(Lerp・Clamp・角度・乱数)の2D版のデモのシーン
class MathDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
