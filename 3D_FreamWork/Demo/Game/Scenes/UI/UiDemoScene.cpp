#include "UiDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/UI/UiDemo.h"

void UiDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"UIデモ(ボタン・スライダー・ゲージ・文字)",
        L"スライダーでゲージが変わり、ボタンでHPや回数が変わります。Anchor は Rect デモで見られます。",
        L"操作: スライダーをドラッグ / ボタンをクリック / Esc: 戻る",
        L"使っているAPI: ButtonComponent, SliderComponent, GaugeRenderer, TextRenderer, SetAnchor" });
    CreateUiDemo(*this);
}

void UiDemoScene::Uninit()
{
    Scene::Uninit();
}
