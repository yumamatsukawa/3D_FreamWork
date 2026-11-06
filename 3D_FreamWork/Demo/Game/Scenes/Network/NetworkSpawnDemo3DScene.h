#pragma once
#include "../../../../Engine/Core/Scene.h"

// スポーン・破棄のデモの3D版のシーン
class NetworkSpawnDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};