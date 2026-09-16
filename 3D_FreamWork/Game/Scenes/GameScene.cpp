#include "GameScene.h"
#include "../Objects/Player.h"
#include "../Objects/Enemy.h"

void GameScene::Init()
{
    CreatePlayer(*this);
    CreateEnemy(*this);
}

void GameScene::Uninit()
{
    Scene::Uninit();
}
