#pragma once
#include "../../../../Engine/Core/Scene.h"

// 物の頭上に出るUI(WorldUI)の3D版のデモのシーン
class WorldUiDemo3DScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
