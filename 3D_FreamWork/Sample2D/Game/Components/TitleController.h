#pragma once
#include "../../../Engine/Component.h"

// タイトル画面の表示とBGMを担当する
class TitleController : public Component {
    unsigned int audioID;   // 再生中のBGM
public:
    void Init() override;
    void Draw() override;
    void Uninit() override;
};
