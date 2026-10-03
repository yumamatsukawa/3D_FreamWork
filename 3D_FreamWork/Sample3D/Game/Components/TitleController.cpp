#include "TitleController.h"
#include "../../../Engine/Audio.h"
#include "../../../Engine/Text.h"

void TitleController::Init(){
    // BGMの再生
    audioID = Audio::Load(L"Assets/TitleBGM.wav", true);
    Audio::Play(audioID);
}

void TitleController::Draw() {
    // タイトル文字
    Text::Draw(L"タイトル(3Dサンプル)", 0, 0, 100);
}

void TitleController::Uninit() {
    // BGMの解放
    Audio::Release(audioID);
}
