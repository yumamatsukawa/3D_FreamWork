#pragma once
#include "../../../../Engine/Core/Scene.h"

// 位置・回転・大きさ・親子(Transform)の2D版のデモのシーン
class TransformDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
