#pragma once
#include "../../../../Engine/Core/Scene.h"

// GameObject と Component の付け外し(Object)の2D版のデモのシーン
class ObjectDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
