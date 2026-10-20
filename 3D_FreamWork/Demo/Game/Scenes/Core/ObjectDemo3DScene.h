#pragma once
#include "../../../../Engine/Core/Scene.h"

// GameObject と Component の付け外し(Object)の3D版のデモのシーン
class ObjectDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
