#include "TitleScene.h"
#include "../Objects/Title.h"

void TitleScene::Init()
{
    AddObject<Title>();
}

void TitleScene::Uninit()
{
    Scene::Uninit();
}