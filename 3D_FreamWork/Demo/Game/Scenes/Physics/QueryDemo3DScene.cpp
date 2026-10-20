#include "QueryDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/QueryDemo.h"

void QueryDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"物理の問い合わせのデモ(Query)",
        L"線を飛ばして当たった物を調べる Raycast と、範囲に入っている物を探す Overlap を、マウスで試します。",
        L"操作: マウス 向き・範囲 / 1 Raycast / 2 OverlapSphere / 3 OverlapBox / 4 タグ / 5 トリガー / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: Physics::Raycast, OverlapSphere, OverlapBox, RaycastHit, QueryTriggers" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 200.0f }, 0.0f, -55.0f, 1500.0f);
    CreateQueryDemo(*this, DemoMode::Mode3D);
}

void QueryDemo3DScene::Uninit()
{
    Scene::Uninit();
}
