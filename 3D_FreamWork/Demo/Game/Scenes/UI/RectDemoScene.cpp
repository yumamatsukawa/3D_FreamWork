#include "RectDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/UI/RectDemo.h"

void RectDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Rectデモ(四角とアンカー)",
        L"画面の9か所の基準点(Anchor)に印と四角を置きます。画面サイズが変わっても、隅や中央からの距離は変わりません。",
        L"操作: 下の3つの四角をドラッグして動かす",
        L"使っているAPI: Image::DrawRect, Anchor, GetAnchoredPosition, GetAnchoredCenter, Graphics::width / height" });
    CreateRectDemo(*this);
}

void RectDemoScene::Uninit()
{
    Scene::Uninit();
}
