#include "DemoMenuScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoMenu.h"

void DemoMenuScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Engine Demo: カテゴリ一覧",
        L"見たい分野のボタンを押すと、その分野の機能が並びます。右下のボタンか Tab で、2D と 3D のどちらを見るかを選びます。",
        L"操作: マウスでボタンをクリック / Tab: 2D と 3D の切り替え / Esc: アプリを終了",
        L"使っているAPI: ButtonComponent, SceneManager::ChangeScene, FadeTransition" }, DemoScreen::Top);
    CreateDemoMenu(*this);
}

void DemoMenuScene::Uninit()
{
    Scene::Uninit();
}
