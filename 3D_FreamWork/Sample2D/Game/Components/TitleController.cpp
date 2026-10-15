#include "TitleController.h"
#include "../../../Engine/Audio.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/Image.h"

void TitleController::Init(){
    // カメラを原点に戻す(ゲーム画面から戻った時、PlayerのCameraFollowComponentが追いかけていた位置のままになっているため)
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;

    // BGMの再生
    audioID = Audio::Load(L"Assets/TitleBGM.wav", true);
    Audio::Play(audioID);

    // フォントの読み込み(同じファイルは2回読まないので、タイトルに戻る度に呼んでよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void TitleController::Draw() {
    // タイトル文字(画面の中央)
    TextStyle style;
    style.size = 100.f;
    style.fontName = fontName;
    Text::Draw(L"タイトル(2Dサンプル)", 0.f, 0.f, style);
}

void TitleController::Uninit() {
    // BGMの解放
    Audio::Release(audioID);
}
