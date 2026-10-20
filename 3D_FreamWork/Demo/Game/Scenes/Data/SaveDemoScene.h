#pragma once
#include "../../../../Engine/Core/Scene.h"

// Save(セーブデータ)のデモのシーン(2Dと3Dの区別が無い機能)
class SaveDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};