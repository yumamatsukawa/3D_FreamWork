#pragma once
#include "../../../../Engine/Core/Scene.h"

// 補間(TweenComponent / Ease)の3D版のデモのシーン
class TweenDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
