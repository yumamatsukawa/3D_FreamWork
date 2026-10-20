#pragma once
#include "../../../../Engine/Core/Scene.h"

// 時間の倍率・ポーズ・Timer(Time)のデモのシーン(2Dと3Dの区別が無い機能)
class TimeDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
