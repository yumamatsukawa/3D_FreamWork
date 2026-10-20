#include "ColliderDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/ColliderDemo.h"

void ColliderDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"当たり判定の種類のデモ(Collider)",
        L"球・箱・カプセルの当たり判定です。クリックで上から物を落として、固定・往復する台・動く物の違いと、枠線を見ます。",
        L"操作: クリック 物を落とす / 1・2・3 形 / F1 枠線 / C 全部消す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: Sphere/Box/CapsuleRigidbodyComponent, BodyType, RigidbodyComponent::SetDebugDrawEnabled" });
    CreateDemoStage3D(*this, { 0.0f, 50.0f, 100.0f }, 0.0f, -35.0f, 1200.0f);
    CreateColliderDemo(*this, DemoMode::Mode3D);
}

void ColliderDemo3DScene::Uninit()
{
    Scene::Uninit();
}
