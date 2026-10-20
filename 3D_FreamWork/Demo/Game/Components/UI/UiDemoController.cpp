#include "UiDemoController.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/UI/SliderComponent.h"
#include "../../../../Engine/UI/TextRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include <cstdio>

void UiDemoController::Setup(SliderComponent* valueSlider, TextRenderer* countText) {
    slider = valueSlider;
    counterText = countText;
}

void UiDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void UiDemoController::AddCount() {
    count++;
    if (counterText) {
        wchar_t buf[32];
        swprintf_s(buf, L"Count: %d", count);
        counterText->SetText(buf);
    }
}

void UiDemoController::ChangeHp(float amount) {
    hp = Math::Clamp(hp + amount, 0.f, maxHp);
}

void UiDemoController::Draw() {
    // 文字の見た目: 黒い縁取り
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.size = 22.f;

    // スライダーの値(スライダーの右)と、HPの数値(HPゲージの右)。スライダーとゲージの位置は Objects/UI/UiDemo.cpp
    wchar_t buf[48];
    style.align = TextAlign::Left;
    style.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (slider) {
        swprintf_s(buf, L"%.2f", slider->GetValue());
        Text::Draw(buf, -125.f, 100.f, style);
    }
    swprintf_s(buf, L"HP %d / %d", (int)hp, (int)maxHp);
    Text::Draw(buf, -125.f, -30.f, style);
}
