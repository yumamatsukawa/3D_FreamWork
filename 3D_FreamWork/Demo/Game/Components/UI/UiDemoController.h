#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class SliderComponent;
class TextRenderer;

// UIデモの値(カウント・HP)と、数値の表示を担当する(ボタンやスライダーは別のオブジェクトで、押された時にここを呼ぶ)
class UiDemoController : public Component {
private:
    static constexpr float maxHp = 100.f;   // HPの最大値

    int count = 0;                              // 「Count」ボタンを押した回数
    float hp = maxHp;                           // 今のHP(ゲージはこの値を読んで表示するだけ)
    SliderComponent* slider = nullptr;          // 値を表示するスライダー
    TextRenderer* counterText = nullptr;        // 回数を表示する文字(オブジェクトに付いて動く文字)
    std::wstring fontName;                      // 文字のフォント(InitでText::LoadFontした名前)

public:
    // 表示に使うスライダーと文字を、CreateUiDemo()から渡してもらう
    void Setup(SliderComponent* valueSlider, TextRenderer* countText);

    // ゲージの値の持ち主はここ(ゲージは読むだけ)
    float GetHp() const { return hp; }
    float GetMaxHp() const { return maxHp; }

    void AddCount();            // 回数を1増やして、文字を変える
    void ChangeHp(float amount);   // HPを増やす(減らす時はマイナス。0〜最大に収める)

    void Init() override;
    void Draw() override;
};
