#pragma once
#include "../../../../Engine/Core/Scene.h"

// ObjectPool(弾を使い回す)のデモのシーン(2Dと3Dの区別が無い機能)
class PoolDemoScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;
};
