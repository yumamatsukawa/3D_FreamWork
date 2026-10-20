#include "RigidbodySettingsDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/RigidbodySettingsDemo.h"

void RigidbodySettingsDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"剛体の設定のデモ(RigidbodySettings)",
        L"上の箱(A)はふつう、下の箱(B)は設定を変えてあります。同じ力で蹴って、動きの違いを比べます。",
        L"操作: 1 質量 / 2 抵抗 / 3 重力 / 4 回転固定 / 5 位置固定(もう一度で軸) / SPACE 蹴る / R 戻す / Tab: 3D版へ",
        L"使っているAPI: RigidbodyComponent::SetMass, SetDrag, SetUseGravity, SetFreezeRotation, SetFreezePositionX/Y/Z, AddForce" });
    CreateRigidbodySettingsDemo(*this, DemoMode::Mode2D);
}

void RigidbodySettingsDemo2DScene::Uninit()
{
    Scene::Uninit();
}
