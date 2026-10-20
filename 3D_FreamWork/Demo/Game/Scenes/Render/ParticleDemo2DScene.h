#pragma once
#include "../../../../Engine/Core/Scene.h"

// パーティクル(ParticleRenderer / CreateEffect)の2D版のデモのシーン
class ParticleDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
