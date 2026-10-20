#pragma once
#include "../../../../Engine/Core/Scene.h"

// カメラ(CameraFollowComponent / CameraShake)の2D版のデモのシーン
class CameraDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
