#pragma once
#include "../../../../Engine/Core/Scene.h"

// スポーン・破棄のデモの2D版のシーン
class NetworkSpawnDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};