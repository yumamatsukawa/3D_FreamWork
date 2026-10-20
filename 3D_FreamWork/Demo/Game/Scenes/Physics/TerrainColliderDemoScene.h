#pragma once
#include "../../../../Engine/Core/Scene.h"

// 地形の当たり判定(TerrainRigidbodyComponent)のデモのシーン(3Dだけ。丘を歩いて登る)
class TerrainColliderDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
