#include "TransitionDemoBScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/TransitionDemo.h"

void TransitionDemoBScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Transitionデモ(シーンB)",
        L"こちらは3Dの舞台のシーンです。演出を選ぶと、2Dの背景のシーンAへ戻ります。右ドラッグで視点を回せます。",
        L"操作: 1 Fade(黒) / 2 Fade(白) / 3 丸 / 4 ワイプ / 右ドラッグ 視点 / Esc: 戻る",
        L"使っているAPI: ChangeScene<T>(FadeTransition / SpriteTransition), SetSwitchFrame, Time::IsPaused" });
    CreateDemoStage3D(*this, { 0.0f, 60.0f, 300.0f }, 0.0f, -20.0f, 800.0f);
    CreateTransitionDemo(*this, TransitionPage::B);
}

void TransitionDemoBScene::Uninit()
{
    Scene::Uninit();
}
