#pragma once
#include "../../../../Engine/Core/Scene.h"

// 3Dの世界に貼る文字(WorldTextRenderer)のデモのシーン(3Dだけ)
class WorldTextDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
