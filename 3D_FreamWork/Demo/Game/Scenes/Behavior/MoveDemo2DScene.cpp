#include "MoveDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/MoveDemo.h"

void MoveDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"動きの部品のデモ(Move)",
        L"マウスの目印を追う青と、クリックした場所へ向かう橙。速さと止まる距離を変えて、まっすぐ進む動きを見ます。",
        L"操作: マウス 目標を動かす / クリック Bの目的地 / ↑↓ 速さ / ←→ 止まる距離 / R 戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: ChaseComponent(SetTarget, SetTargetPosition, SetSpeed, SetStopDistance, SetPlane), HasArrived" });
    CreateMoveDemo(*this, DemoMode::Mode2D);
}

void MoveDemo2DScene::Uninit()
{
    Scene::Uninit();
}
