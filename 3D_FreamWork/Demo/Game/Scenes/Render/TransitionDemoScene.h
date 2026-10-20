#pragma once
#include "../../../../Engine/Core/Scene.h"

// シーン切り替えの演出(Transition)のデモのシーンA(2Dの背景。デモ一覧から開く側。Bと行き来する)
class TransitionDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
