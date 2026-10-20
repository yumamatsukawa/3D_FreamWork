#pragma once
#include "../../../../Engine/Core/Scene.h"

// スプライトのコマ送り(SpriteAnimationComponent)の2D版のデモのシーン
class SpriteAnimDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
