#pragma once
#include "../../../../Engine/Core/Scene.h"

// 機能一覧のシーン(今のカテゴリの機能を選ぶ画面。カテゴリごとにシーンは作らず、このシーンが今のカテゴリを読んで並べる)
class DemoCategoryScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
