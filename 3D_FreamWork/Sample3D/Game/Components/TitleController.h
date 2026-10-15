#pragma once
#include "../../../Engine/Component.h"
#include <string>

// タイトル画面の表示とBGMを担当する(ゲーム開始はSTARTボタンの担当)
class TitleController : public Component {
    unsigned int audioID;   // 再生中のBGM
    float volume = 1.f;     // BGMの音量(0〜1。SaveDataに保存して、次に起動した時も残る)
    std::wstring fontName;  // タイトル文字のフォント(InitでText::LoadFontした名前)
public:
    void Init() override;
    void Draw() override;
    void Uninit() override;

    // BGMの音量(音量スライダーが使う。値の持ち主はこのComponent)
    float GetVolume() const { return volume; }
    void SetVolume(float newVolume);   // BGMの音量を変えて、SaveDataに書く(ファイルへの保存はSaveData::Save)
};
