#include "TitleController.h"
#include "../../../Engine/Audio.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/SaveData.h"
#include "../../../Engine/MathUtil.h"

namespace {
    constexpr const char* musicVolumeKey = "MusicVolume";   // BGMの音量を保存するSaveDataのキー(Sample2DとSample3Dで共通)
}

void TitleController::Init(){
    // BGMの再生(音量は前に設定した値。保存されていなければ最大。Sample2DとSample3Dで同じ設定を使う)
    volume = Math::Clamp(SaveData::GetFloat(musicVolumeKey, 1.f), 0.f, 1.f);
    audioID = Audio::Load(L"Assets/TitleBGM.wav", true);
    Audio::Play(audioID, volume);

    // フォントの読み込み(同じファイルは2回読まないので、タイトルに戻る度に呼んでよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void TitleController::Draw() {
    // タイトル文字(画面の中央)
    TextStyle style;
    style.size = 100.f;
    style.fontName = fontName;
    Text::Draw(L"タイトル(3Dサンプル)", 0.f, 0.f, style);
}

void TitleController::SetVolume(float newVolume) {
    // 0〜1に収めて、鳴っているBGMの音量を変え、SaveDataに書く(ファイルに保存するのは、スライダーを離した時のSaveData::Save)
    volume = Math::Clamp(newVolume, 0.f, 1.f);
    Audio::SetVolume(audioID, volume);
    SaveData::SetFloat(musicVolumeKey, volume);
}

void TitleController::Uninit() {
    // BGMの解放
    Audio::Release(audioID);
}
