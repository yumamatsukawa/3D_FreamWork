#include "ColliderDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/ColliderDemo.h"

void ColliderDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"当たり判定の種類のデモ(Collider)",
        L"円・四角・カプセルの当たり判定です。クリックで物を出して、固定・往復する台・動く物の違いと、枠線を見ます。",
        L"操作: クリック 物を出す / 1・2・3 形 / F1 枠線 / C 全部消す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: Circle/Square/Capsule2DRigidbodyComponent, BodyType, RigidbodyComponent::SetDebugDrawEnabled" });
    CreateColliderDemo(*this, DemoMode::Mode2D);
}

void ColliderDemo2DScene::Uninit()
{
    Scene::Uninit();
}
