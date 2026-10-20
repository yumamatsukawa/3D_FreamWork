#pragma once
#include "../../../../Engine/Core/Scene.h"

// パーティクル(ParticleRenderer / CreateEffect)の3D版のデモのシーン
class ParticleDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
