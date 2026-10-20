#include "ObjectDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/ObjectDemo.h"

void ObjectDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Objectデモ(GameObject / Component)",
        L"物(GameObject)に動きの部品(Component)を付けたり止めたりして、動きが変わるのを見ます。",
        L"操作: 1 回る部品 / 2 上下に動く部品 / 3 部品を止める・再開 / クリック 物を出す / X 出した物を消す / Tab: 3D版へ",
        L"使っているAPI: Scene::CreateObject, AddComponent, GetComponent, Component::SetEnabled, Destroy" });
    CreateObjectDemo(*this, DemoMode::Mode2D);
}

void ObjectDemo2DScene::Uninit()
{
    Scene::Uninit();
}
