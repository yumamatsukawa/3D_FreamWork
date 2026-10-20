#include "TriggerDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/TriggerDemo.h"

void TriggerDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"トリガーのデモ(Trigger)",
        L"黄色い球はすり抜けるトリガー、青い箱はぶつかる箱です。緑の球を動かして、呼ばれる関数と回数の違いを見ます。",
        L"操作: WASD・矢印 球を動かす / R 回数を0に戻す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: Component::OnTriggerEnter, OnTriggerStay, OnTriggerExit, OnCollisionEnter, isTrigger" });
    GameObject* camera = CreateDemoStage3D(*this, { 0.0f, 200.0f, 0.0f }, 0.0f, -35.0f, 1100.0f);
    CreateTriggerDemo(*this, DemoMode::Mode3D, camera);
}

void TriggerDemo3DScene::Uninit()
{
    Scene::Uninit();
}
