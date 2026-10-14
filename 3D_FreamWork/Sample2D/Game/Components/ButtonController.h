#pragma once
#include "../../../Engine/Component.h"
#include <string>
#include <functional>

// クリックできるボタン(見た目の変化・クリック判定・文字の表示)を担当する
class ButtonController : public Component {
private:
    std::wstring label;               // ボタンに表示する文字
    float textSize;                   // 文字の大きさ
    std::function<void()> onClick;    // クリックされた時の処理
    bool pressed = false;             // ボタンの上で押し始めたか
    float baseHeight = 0.f;           // 作った時のボタンの高さ(文字の大きさをボタンの大きさに合わせる基準)

public:
    ButtonController(const std::wstring& label, float textSize, std::function<void()> onClick)
        : label(label), textSize(textSize), onClick(std::move(onClick)) {}

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
