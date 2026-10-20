#pragma once
#include "../../../../Engine/Core/Scene.h"

// 四角(Image::DrawRect)とアンカー(Anchor)のデモのシーン(2Dと3Dの区別が無い機能)
class RectDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
