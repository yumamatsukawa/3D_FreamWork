#pragma once
#include "../../../Engine/Scene.h"

// ゲーム本編のシーン
class GameScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
