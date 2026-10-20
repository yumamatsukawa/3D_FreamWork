#include "ScreenEffectDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/ScreenEffectDemo.h"

void ScreenEffectDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"ScreenEffectデモ(画面全体の演出)",
        L"画面全体に、光(Flash)・赤い縁の点滅(Vignette)・暗転と明転(Fade)を重ねます。2Dでも3Dでも同じように使えます。",
        L"操作: 1 Flash / 2 Vignette / 3 Fade(黒) / 4 Fade(白) / Esc: 戻る",
        L"使っているAPI: ScreenEffect::Flash, Vignette, Fade(onComplete), IsFading, Stop" });
    CreateScreenEffectDemo(*this);
}

void ScreenEffectDemoScene::Uninit()
{
    Scene::Uninit();
}
