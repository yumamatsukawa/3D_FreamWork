#pragma once
#include "../../../../Engine/Core/Scene.h"

// Csv(表の読み込みとホットリロード)のデモのシーン(2Dと3Dの区別が無い機能)
class CsvDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};