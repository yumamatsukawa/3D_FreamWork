#include "TransitionDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/TransitionDemo.h"

void TransitionDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Transitionデモ(シーン切り替えの演出)",
        L"演出を選ぶと、別のシーン(AとB)へ切り替わります。もう一度押すと戻ります。切り替え中は、ゲームの時間が止まります。",
        L"操作: 1 Fade(黒) / 2 Fade(白) / 3 丸 / 4 ワイプ / Esc: 戻る",
        L"使っているAPI: ChangeScene<T>(FadeTransition / SpriteTransition), SetSwitchFrame, Time::IsPaused" });
    CreateTransitionDemo(*this, TransitionPage::A);
}

void TransitionDemoScene::Uninit()
{
    Scene::Uninit();
}
