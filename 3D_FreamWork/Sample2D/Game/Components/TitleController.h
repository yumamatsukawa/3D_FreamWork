#pragma once
#include "../../../Engine/Component.h"
#include <string>

// タイトル画面の表示とBGMを担当する
class TitleController : public Component {
    unsigned int audioID;   // 再生中のBGM
    std::wstring fontName;  // タイトル文字のフォント(InitでText::LoadFontした名前)
public:
    void Init() override;
    void Draw() override;
    void Uninit() override;
};
