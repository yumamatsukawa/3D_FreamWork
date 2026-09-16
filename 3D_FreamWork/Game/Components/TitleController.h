#pragma once
#include "../../Engine/Component.h"

// タイトル画面の表示とシーン遷移(SPACEキーでゲーム開始)を担当するComponent
class TitleController : public Component {
public:
    void Update(float dt) override;
    void Draw() override;
};
