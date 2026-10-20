#include "MoveDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/MoveDemo.h"

void MoveDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"動きの部品のデモ(Move)",
        L"マウスの目印を追う青と、クリックした場所へ向かう橙。速さと止まる距離を変えて、まっすぐ進む動きを見ます。",
        L"操作: マウス 目標を動かす / クリック Bの目的地 / ↑↓ 速さ / ←→ 止まる距離 / R 戻す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: ChaseComponent(SetTarget, SetTargetPosition, SetSpeed, SetStopDistance, SetPlane), HasArrived" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 60.0f }, 0.0f, -58.0f, 1250.0f);
    CreateMoveDemo(*this, DemoMode::Mode3D);
}

void MoveDemo3DScene::Uninit()
{
    Scene::Uninit();
}
