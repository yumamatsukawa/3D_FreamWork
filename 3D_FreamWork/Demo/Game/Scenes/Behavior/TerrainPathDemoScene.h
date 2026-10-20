#pragma once
#include "../../../../Engine/Core/Scene.h"

// 丘の経路探索のデモのシーン(3D版だけ)
class TerrainPathDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
