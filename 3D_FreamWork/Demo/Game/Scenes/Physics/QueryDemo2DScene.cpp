#include "QueryDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Physics/QueryDemo.h"

void QueryDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"物理の問い合わせのデモ(Query)",
        L"線を飛ばして当たった物を調べる Raycast と、範囲に入っている物を探す Overlap を、マウスで試します。",
        L"操作: マウス 向き・範囲 / 1 Raycast / 2 OverlapSphere / 3 OverlapBox / 4 タグ / 5 トリガー / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: Physics::Raycast, OverlapSphere, OverlapBox, RaycastHit, QueryTriggers" });
    CreateQueryDemo(*this, DemoMode::Mode2D);
}

void QueryDemo2DScene::Uninit()
{
    Scene::Uninit();
}
