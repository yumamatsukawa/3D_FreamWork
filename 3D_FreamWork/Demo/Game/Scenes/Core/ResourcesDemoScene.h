#pragma once
#include "../../../../Engine/Core/Scene.h"

// Resources(画像・音・モデルの共有)のデモのシーン(2Dと3Dの区別が無い機能)
class ResourcesDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
