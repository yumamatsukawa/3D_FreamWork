#pragma once
#include "../../../../Engine/Core/Component.h"
#include "DemoGuideController.h"
#include <string>

class ButtonComponent;

// 2D/3D切り替えボタンの文字と、押せるか(暗いか)を、毎フレーム今の状態に合わせる(同じオブジェクトの ButtonComponent を操作する)
class DemoModeButtonController : public Component {
private:
    DemoScreen screen;                      // どの画面のボタンか
    ButtonComponent* button = nullptr;      // 操作するボタン(同じオブジェクトに付いている)
    std::wstring lastLabel;                 // 前に反映した文字
    int lastInteractable = -1;              // 前に反映した「押せるか」(-1=まだ反映していない)

public:
    explicit DemoModeButtonController(DemoScreen buttonScreen) : screen(buttonScreen) {}

    void Init() override;
    void Update(float dt) override;
};
