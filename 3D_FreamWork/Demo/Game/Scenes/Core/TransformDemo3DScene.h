#pragma once
#include "../../../../Engine/Core/Scene.h"

// 位置・回転・大きさ・親子(Transform)の3D版のデモのシーン
class TransformDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
