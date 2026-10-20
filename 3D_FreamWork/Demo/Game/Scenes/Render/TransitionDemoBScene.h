#pragma once
#include "../../../../Engine/Core/Scene.h"

// シーン切り替えの演出(Transition)のデモのシーンB(3Dの舞台。Aと行き来する。デモ一覧には出ない)
class TransitionDemoBScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
