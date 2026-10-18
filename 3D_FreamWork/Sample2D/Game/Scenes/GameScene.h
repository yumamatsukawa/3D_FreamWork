#pragma once
#include "../../../Engine/Core/Scene.h"

// ゲーム本編のシーン
class GameScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
