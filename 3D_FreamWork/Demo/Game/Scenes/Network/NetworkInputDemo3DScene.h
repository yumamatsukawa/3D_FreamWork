#pragma once
#include "../../../../Engine/Core/Scene.h"

// 入力の同期のデモの3D版のシーン
class NetworkInputDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};