#pragma once
#include "../../../../Engine/Core/Scene.h"

// 遠い物を省く設定(LOD)のデモのシーン(3Dだけ)
class LodDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
