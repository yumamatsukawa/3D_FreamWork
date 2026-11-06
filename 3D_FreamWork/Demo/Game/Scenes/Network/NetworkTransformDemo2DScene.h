#pragma once
#include "../../../../Engine/Core/Scene.h"

// 位置の同期のデモの2D版のシーン
class NetworkTransformDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};