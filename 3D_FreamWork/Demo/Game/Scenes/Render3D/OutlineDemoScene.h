#pragma once
#include "../../../../Engine/Core/Scene.h"

// 枠線と縁の光のデモのシーン(3Dだけ)
class OutlineDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
