#pragma once
#include "../../../../Engine/Core/Scene.h"

// UI(ボタン・スライダー・ゲージ・文字・アンカー)のデモのシーン(2D・3Dの区別は無く、どちらの側でも同じ)
class UiDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
