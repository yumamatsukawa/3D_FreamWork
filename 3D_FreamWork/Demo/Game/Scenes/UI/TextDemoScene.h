#pragma once
#include "../../../../Engine/Core/Scene.h"

// 文字(Text / TextRenderer)のデモのシーン(2Dと3Dの区別が無い機能)
class TextDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
