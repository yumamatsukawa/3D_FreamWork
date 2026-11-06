#pragma once
#include "../../../../Engine/Core/Scene.h"

// 位置の同期のデモの3D版のシーン
class NetworkTransformDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};