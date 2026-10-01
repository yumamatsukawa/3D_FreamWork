#include "TitleScene.h"
#include "../Objects/Title.h"
#include "../Objects/StartButton.h"

void TitleScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateTitle(*this);
    CreateStartButton(*this);
}

void TitleScene::Uninit()
{
    Scene::Uninit();
}
