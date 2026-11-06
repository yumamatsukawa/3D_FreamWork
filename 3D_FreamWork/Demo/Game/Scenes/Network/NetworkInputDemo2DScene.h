#pragma once
#include "../../../../Engine/Core/Scene.h"

// 入力の同期のデモの2D版のシーン
class NetworkInputDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};