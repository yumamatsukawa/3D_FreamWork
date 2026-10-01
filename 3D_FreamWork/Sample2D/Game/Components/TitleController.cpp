#include "TitleController.h"
#include "../../../Engine/Audio.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/Image.h"

void TitleController::Init(){
    // カメラを原点に戻す(ゲーム画面から戻った時、Playerを追いかけていた位置のままになっているため)
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;

    // BGMの再生
    audioID = Audio::Load(L"Assets/TitleBGM.wav", true);
    Audio::Play(audioID);
}

void TitleController::Draw() {
    // タイトル文字
    Text::Draw(L"タイトル(2Dサンプル)", 0, 0, 100);
}

void TitleController::Uninit() {
    // BGMの解放
    Audio::Release(audioID);
}
