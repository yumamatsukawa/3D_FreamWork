#include "ForceDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/ForceDemo.h"

void ForceDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"力と速度のデモ(Force)",
        L"軽い箱と重い箱を、同じ大きさの力で押します。4種類の力(ForceMode)と、SetVelocity・AddTorque の違いを比べます。",
        L"操作: 1〜4 力の種類 / SPACE 力をかける / V 速度を上書き / T 回す / R 戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: RigidbodyComponent::AddForce, AddTorque, SetVelocity, GetVelocity, SetMass, SetDrag, ForceMode" });
    CreateForceDemo(*this, DemoMode::Mode2D);
}

void ForceDemo2DScene::Uninit()
{
    Scene::Uninit();
}
