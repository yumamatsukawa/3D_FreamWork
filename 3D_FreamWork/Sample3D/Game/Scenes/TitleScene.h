#pragma once
#include "../../../Engine/Core/Scene.h"

// タイトル画面のシーン
class TitleScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
