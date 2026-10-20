#include "ObjectDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/ObjectDemo.h"

void ObjectDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Objectデモ(GameObject / Component)",
        L"立方体(GameObject)に動きの部品(Component)を付けたり止めたりして、動きが変わるのを見ます。",
        L"操作: 1 回る部品 / 2 上下に動く部品 / 3 止める・再開 / クリック 物を出す / X 消す / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: Scene::CreateObject, AddComponent, GetComponent, Component::SetEnabled, Destroy" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 300.0f }, 0.0f, -25.0f, 700.0f);
    CreateObjectDemo(*this, DemoMode::Mode3D);
}

void ObjectDemo3DScene::Uninit()
{
    Scene::Uninit();
}
