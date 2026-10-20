#pragma once
#include "../../../../Engine/Core/Scene.h"

// カメラ(三人称・一人称 / CameraShake)の3D版のデモのシーン
class CameraDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
