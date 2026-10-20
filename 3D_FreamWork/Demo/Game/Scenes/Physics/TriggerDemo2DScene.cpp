#include "TriggerDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/TriggerDemo.h"

void TriggerDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"トリガーのデモ(Trigger)",
        L"黄色い丸はすり抜けるトリガー、青い箱はぶつかる箱です。球を動かして、呼ばれる関数と回数の違いを見ます。",
        L"操作: WASD・矢印 球を動かす / R 回数を0に戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: Component::OnTriggerEnter, OnTriggerStay, OnTriggerExit, OnCollisionEnter, isTrigger" });
    CreateTriggerDemo(*this, DemoMode::Mode2D);
}

void TriggerDemo2DScene::Uninit()
{
    Scene::Uninit();
}
