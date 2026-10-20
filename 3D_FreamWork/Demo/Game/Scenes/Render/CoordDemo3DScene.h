#pragma once
#include "../../../../Engine/Core/Scene.h"

// 座標の変換(Image::WorldToScreen3D / ScreenPointToRay3D)の3D版のデモのシーン
class CoordDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
