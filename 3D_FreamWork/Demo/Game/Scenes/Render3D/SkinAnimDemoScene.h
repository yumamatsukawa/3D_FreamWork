#pragma once
#include "../../../../Engine/Core/Scene.h"

// 3Dアニメーション(骨で曲がるモデル)のデモのシーン(3Dだけ)
class SkinAnimDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
