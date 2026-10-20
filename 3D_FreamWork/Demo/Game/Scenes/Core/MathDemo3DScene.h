#pragma once
#include "../../../../Engine/Core/Scene.h"

// MathUtil(Lerp・内積・外積・Normalize・MoveTowards)の3D版のデモのシーン
class MathDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
