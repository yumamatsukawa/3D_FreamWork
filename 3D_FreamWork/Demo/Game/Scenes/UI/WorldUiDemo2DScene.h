#pragma once
#include "../../../../Engine/Core/Scene.h"

// 物の頭上に出るUI(WorldUI)の2D版のデモのシーン
class WorldUiDemo2DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
