#include "RigidbodySettingsDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/RigidbodySettingsDemo.h"

void RigidbodySettingsDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"剛体の設定のデモ(RigidbodySettings)",
        L"手前の箱(A)はふつう、奥の箱(B)は設定を変えてあります。同じ力で蹴って、動きの違いを比べます。",
        L"操作: 1 質量 / 2 抵抗 / 3 重力 / 4 回転固定 / 5 位置固定(もう一度で軸) / SPACE 蹴る / R 戻す / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: RigidbodyComponent::SetMass, SetDrag, SetUseGravity, SetFreezeRotation, SetFreezePositionX/Y/Z, AddForce" });
    CreateDemoStage3D(*this, { 0.0f, 200.0f, 0.0f }, 0.0f, -30.0f, 1100.0f);
    CreateRigidbodySettingsDemo(*this, DemoMode::Mode3D);
}

void RigidbodySettingsDemo3DScene::Uninit()
{
    Scene::Uninit();
}
