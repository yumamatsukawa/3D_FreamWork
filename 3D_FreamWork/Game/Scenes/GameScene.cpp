#include "GameScene.h"
#include "../Objects/Player.h"
#include "../Objects/Enemy.h"

void GameScene::Init()
{
    AddObject<Player>("Player");
    AddObject<Enemy>("Enemy");
}

void GameScene::Uninit()
{
    Scene::Uninit();
}