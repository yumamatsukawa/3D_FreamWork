#pragma once
#include "../../../../Engine/Core/Scene.h"

// Sound(BGM・SE・音量グループ・フェード)のデモのシーン(2Dと3Dの区別が無い機能)
class SoundDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
