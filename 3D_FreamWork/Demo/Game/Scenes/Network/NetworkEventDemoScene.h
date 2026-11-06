#pragma once
#include "../../../../Engine/Core/Scene.h"

// 出来事(スイッチを押すと扉が開く)のデモのシーン(2Dと3Dの区別が無い機能)
class NetworkEventDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};