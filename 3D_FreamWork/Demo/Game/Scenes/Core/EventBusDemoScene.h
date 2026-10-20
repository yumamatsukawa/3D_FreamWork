#pragma once
#include "../../../../Engine/Core/Scene.h"

// EventBus(互いを知らない管理役のやり取り)のデモのシーン(2Dと3Dの区別が無い機能)
class EventBusDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
