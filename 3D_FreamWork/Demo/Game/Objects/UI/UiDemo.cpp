#include "UiDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/UI/UiDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/UI/SliderComponent.h"
#include "../../../../Engine/UI/GaugeRenderer.h"
#include "../../../../Engine/UI/TextRenderer.h"
#include "../../../../Engine/UI/Text.h"

GameObject* CreateUiDemo(Scene& scene) {
    // 値の持ち主(カウント・HP)と、数値の表示
    GameObject* demo = scene.CreateObject("UiDemo");
    UiDemoController* controller = demo->AddComponent<UiDemoController>();

    // ── 左の列: スライダーとゲージ ──

    // スライダー(画面の中央から左へ300、上へ100。値は0〜1)
    SliderComponent* slider = demo->AddComponent<SliderComponent>();
    slider->SetRange(0.f, 1.f);
    slider->SetValue(0.5f);
    slider->SetAnchor(Anchor::Center, { -300.f, 100.f });
    slider->SetSize(300.f, 12.f);
    TextStyle labelStyle;
    labelStyle.size = 24.f;
    labelStyle.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    slider->SetLabel(L"Slider", labelStyle);

    // スライダーの下のゲージ(スライダーの値を毎フレーム読んで表示するだけ。値の持ち主はスライダー)
    GaugeRenderer* sliderGauge = demo->AddComponent<GaugeRenderer>();
    sliderGauge->SetAnchor(Anchor::Center, { -300.f, 40.f });
    sliderGauge->SetSize(300.f, 24.f);
    sliderGauge->SetColors({ 0.3f, 0.85f, 0.3f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.6f });
    sliderGauge->SetValueSource([slider] { return slider->GetValue(); }, [] { return 1.f; });

    // HPゲージ(減った分が少し遅れて縮む。値の持ち主はUiDemoController)
    GaugeRenderer* hpGauge = demo->AddComponent<GaugeRenderer>();
    hpGauge->SetAnchor(Anchor::Center, { -300.f, -30.f });
    hpGauge->SetSize(300.f, 24.f);
    hpGauge->SetColors({ 1.0f, 0.4f, 0.4f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.6f });
    hpGauge->SetDelayedBar(true, { 1.0f, 0.9f, 0.4f, 1.0f });
    hpGauge->SetValueSource([controller] { return controller->GetHp(); }, [controller] { return controller->GetMaxHp(); });

    // HPを増減するボタン(押されたら、値の持ち主に頼む)
    CreateDemoButton(scene, L"Damage", { -375.f, -110.f }, { 140.f, 48.f }, 24.f, [controller] { controller->ChangeHp(-20.f); });
    CreateDemoButton(scene, L"Heal", { -225.f, -110.f }, { 140.f, 48.f }, 24.f, [controller] { controller->ChangeHp(20.f); });

    // ── 右の列: ボタンの回数と、物に付いて動く文字 ──

    // 回数を数えるボタン
    CreateDemoButton(scene, L"Count UP", { 300.f, 100.f }, { 240.f, 72.f }, 32.f, [controller] { controller->AddCount(); });

    // 回数の文字(TextRenderer。オブジェクトの位置に出る文字)
    TextStyle counterStyle;
    counterStyle.size = 36.f;
    counterStyle.fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
    counterStyle.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    GameObject* counter = scene.CreateObject("CounterText");
    counter->transform.position = { 300.f, 40.f, 0.f };
    TextRenderer* counterText = counter->AddComponent<TextRenderer>();
    counterText->SetText(L"Count: 0");
    counterText->SetStyle(counterStyle);

    // 左右に動く丸と、それに付いて動く文字(TextRendererはオブジェクトと一緒に動く)
    GameObject* follower = scene.CreateObject("TextFollower");
    follower->transform.position = { 200.f, -80.f, 0.f };
    follower->transform.scale = { 36.f, 36.f, 1.f };
    follower->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(2);
    follower->AddComponent<TweenComponent>()->MoveTo({ 400.f, -80.f, 0.f }, 1.5f, Ease::InOutQuad).SetLoop(true, true);
    TextStyle followerStyle;
    followerStyle.size = 22.f;
    followerStyle.fontName = counterStyle.fontName;
    followerStyle.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    TextRenderer* followerText = follower->AddComponent<TextRenderer>();
    followerText->SetText(L"TextRenderer");
    followerText->SetStyle(followerStyle);
    followerText->SetOffset({ 0.f, 36.f, 0.f });

    // 表示に使う物を、値の持ち主に渡す
    controller->Setup(slider, counterText);
    return demo;
}
