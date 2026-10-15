#include "TitleScene.h"
#include "../Objects/Title.h"
#include "../Objects/StartButton.h"
#include "../Objects/VolumeSlider.h"
#include "../Components/TitleController.h"
#include "../../../Engine/GameObject.h"

void TitleScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    GameObject* title = CreateTitle(*this);
    CreateStartButton(*this);
    CreateVolumeSlider(*this, title->GetComponent<TitleController>());
}

void TitleScene::Uninit()
{
    Scene::Uninit();
}
