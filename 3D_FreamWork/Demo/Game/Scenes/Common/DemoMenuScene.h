#pragma once
#include "../../../../Engine/Core/Scene.h"

// カテゴリ一覧のシーン(起動した時に最初に出る画面)
class DemoMenuScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
