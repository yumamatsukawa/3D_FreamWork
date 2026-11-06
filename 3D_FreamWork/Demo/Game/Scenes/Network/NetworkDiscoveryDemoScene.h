#pragma once
#include "../../../../Engine/Core/Scene.h"

// LANのホスト探索のデモのシーン(2Dと3Dの区別が無い機能)
class NetworkDiscoveryDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};