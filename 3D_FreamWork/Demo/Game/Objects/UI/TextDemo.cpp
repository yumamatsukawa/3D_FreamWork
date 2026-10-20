#include "TextDemo.h"
#include "../../Components/UI/TextDemoController.h"
#include "../../Components/Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/UI/TextRenderer.h"

GameObject* CreateTextDemo(Scene& scene) {
    // 左右に動く丸と、それに付いて動く文字(TextRendererはオブジェクトと一緒に動く。文字は物の少し上に出す)
    GameObject* mover = scene.CreateObject("TextMover");
    mover->transform.position = { 100.f, -110.f, 0.f };
    mover->transform.scale = { 40.f, 40.f, 1.f };
    mover->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(3);
    mover->AddComponent<TweenComponent>()->MoveTo({ 560.f, -110.f, 0.f }, 2.0f, Ease::InOutQuad).SetLoop(true, true);
    TextRenderer* moverText = mover->AddComponent<TextRenderer>();
    moverText->SetStyle(MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }, TextAlign::Center));
    moverText->SetOffset({ 0.f, 36.f, 0.f });

    // 文字の見本の表示とキー入力をする役(動く物の文字の中身も変える)
    GameObject* demo = scene.CreateObject("TextDemo");
    demo->AddComponent<TextDemoController>(moverText);
    return demo;
}
