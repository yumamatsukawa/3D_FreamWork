#include "TitleController.h"
#include "../../../Engine/Audio/Audio.h"
#include "../../../Engine/UI/Text.h"
#include "../../../Engine/Data/SaveData.h"
#include "../../../Engine/Core/MathUtil.h"
#include "../../../Engine/Render/Image.h"

namespace {
    constexpr const char* musicVolumeKey = "MusicVolume";   // BGMの音量を保存するSaveDataのキー(Sample2DとSample3Dで共通)
    constexpr const char* soundEffectVolumeKey = "SoundEffectVolume";   // 効果音(SE)の音量を保存するSaveDataのキー(音量を変えるUIはまだ無い)
}

void TitleController::Init(){
    // カメラを原点に戻す(ゲーム画面から戻った時、PlayerのCameraFollowComponentが追いかけていた位置のままになっているため)
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;

    // 音量の適用(BGM・効果音とも前に設定した値。保存されていなければ最大。Sample2DとSample3Dで同じ設定を使う)
    // グループの音量なので、ここで決めれば、ゲーム画面の効果音(PlayOneShotAt)にも効く
    volume = Math::Clamp(SaveData::GetFloat(musicVolumeKey, 1.f), 0.f, 1.f);
    Audio::SetGroupVolume(AudioGroup::BGM, volume);
    Audio::SetGroupVolume(AudioGroup::SE, Math::Clamp(SaveData::GetFloat(soundEffectVolumeKey, 1.f), 0.f, 1.f));

    // BGMの再生(BGMグループなので、音量はグループ音量で決まる)
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

void TitleController::SetVolume(float newVolume) {
    // 0〜1に収めて、BGMグループの音量を変え、SaveDataに書く(ファイルに保存するのは、スライダーを離した時のSaveData::Save)
    volume = Math::Clamp(newVolume, 0.f, 1.f);
    Audio::SetGroupVolume(AudioGroup::BGM, volume);
    SaveData::SetFloat(musicVolumeKey, volume);
}

void TitleController::Uninit() {
    // BGMの解放
    Audio::Release(audioID);
}
