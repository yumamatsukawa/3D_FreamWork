#pragma once
#include "../../../../Engine/Core/Scene.h"

// キーボード・マウス・ゲームパッド(Input)のデモのシーン(2Dと3Dの区別が無い機能)
class InputDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
