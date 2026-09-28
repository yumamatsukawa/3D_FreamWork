#include "TitleController.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/Audio.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/SceneManager.h"
#include "../Scenes/GameScene.h"

void TitleController::Init(){
    audioID = Audio::Load(L"Assets/TitleBGM.wav", true);
    Audio::Play(audioID);
}

void TitleController::Update(float dt) {
    if (Input::GetKeyPress(KEY_SPACE)) {
        SceneManager::Get().ChangeScene<GameScene>();
    }
}

void TitleController::Draw() {
    Text::Draw(L"タイトル(3Dサンプル)", 0, 0, 100);
}

void TitleController::Uninit() {
    Audio::Release(audioID);
}
