#pragma once
#include "../../Engine/Scene.h"
#include "../Objects/BulletManager.h"

class GameScene : public Scene
{
public:
    void Init()           override;
    void Uninit()         override;

    // このシーン内で撃たれる弾のプールを管理する。
    // GameSceneのメンバなので、シーンが破棄される時に一緒に破棄され、
    // ダングリングポインタが残る心配が無い
    BulletManager bulletManager;
};