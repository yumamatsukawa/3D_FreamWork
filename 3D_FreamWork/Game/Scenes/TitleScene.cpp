#include "TitleScene.h"
#include "../Objects/Title.h"

void TitleScene::Init()
{
    CreateTitle(*this);
}

void TitleScene::Uninit()
{
    Scene::Uninit();
}
