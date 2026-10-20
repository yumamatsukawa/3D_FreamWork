#pragma once
#include "../../../../Engine/Core/Scene.h"

// 画面全体の演出(ScreenEffect)のデモのシーン(2Dと3Dの区別が無い)
class ScreenEffectDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
