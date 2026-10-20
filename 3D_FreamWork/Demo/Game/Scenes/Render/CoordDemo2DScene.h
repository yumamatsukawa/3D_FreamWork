#pragma once
#include "../../../../Engine/Core/Scene.h"

// 座標の変換(Image::WorldToScreen2D / ScreenToWorld2D)の2D版のデモのシーン
class CoordDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
