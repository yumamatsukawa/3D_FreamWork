#pragma once
#include "../../../../Engine/Core/Scene.h"

// 補間(TweenComponent / Ease)の2D版のデモのシーン
class TweenDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
