#pragma once
#include "../../../../Engine/Core/Scene.h"

// 地形の見た目(Terrain / TerrainRenderer)のデモのシーン(3Dだけ)
class TerrainDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
