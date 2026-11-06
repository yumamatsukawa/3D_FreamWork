#pragma once
#include "../../../../Engine/Core/Scene.h"

// ネットワークの基本(つなぐ・文字を送る)のデモのシーン(2Dと3Dの区別が無い機能)
class NetworkBasicDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};