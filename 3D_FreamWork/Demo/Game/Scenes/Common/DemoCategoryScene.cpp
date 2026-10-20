#include "DemoCategoryScene.h"
#include "DemoRegistry.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCategoryMenu.h"

void DemoCategoryScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { std::wstring(L"機能一覧: ") + GetCategoryInfo(GetCurrentCategory()).name,
        L"見たい機能のボタンを押すと、その機能のデモが始まります。暗いボタンは、今の側(2D/3D)の版が無い機能です。",
        L"操作: マウスでボタンをクリック / Tab: 2D と 3D の切り替え / Esc: カテゴリ一覧へ戻る",
        L"使っているAPI: ButtonComponent, SceneManager::ChangeScene, FadeTransition" }, DemoScreen::List);
    CreateDemoCategoryMenu(*this);
}

void DemoCategoryScene::Uninit()
{
    Scene::Uninit();
}
