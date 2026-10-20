#include "ForceDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/ForceDemo.h"

void ForceDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"力と速度のデモ(Force)",
        L"軽い箱と重い箱を、同じ大きさの力で押します(重力は切ってあります)。4種類の力と、SetVelocity・AddTorque を比べます。",
        L"操作: 1〜4 力の種類 / SPACE 力をかける / V 速度 / T 回す / R 戻す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: RigidbodyComponent::AddForce, AddTorque, SetVelocity, GetVelocity, SetMass, SetDrag, ForceMode" });
    CreateDemoStage3D(*this, { 0.0f, 250.0f, 0.0f }, 0.0f, -25.0f, 1100.0f);
    CreateForceDemo(*this, DemoMode::Mode3D);
}

void ForceDemo3DScene::Uninit()
{
    Scene::Uninit();
}
