#pragma once
#include "../../../../Engine/Core/Scene.h"

// スプライトのコマ送り(SpriteAnimationComponent)の3D版のデモのシーン
class SpriteAnimDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
