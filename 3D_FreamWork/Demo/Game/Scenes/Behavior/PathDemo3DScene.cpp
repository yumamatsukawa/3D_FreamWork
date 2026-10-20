#include "PathDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/PathDemo.h"

void PathDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"経路探索のデモ(Path)",
        L"緑は格子の上で経路を探して壁を避け、赤は目的地へまっすぐ向かうだけで壁に止まります。クリックで目的地を動かします。",
        L"操作: クリック 目的地 / ↑↓ 速さ / ←→ 探し直す間隔 / F1 格子と経路の線 / R 戻す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: NavGridComponent, PathFollowComponent(SetNavigation, SetRepathInterval, HasPath), SetDebugDrawEnabled" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 60.0f }, 0.0f, -58.0f, 1250.0f);
    CreatePathDemo(*this, DemoMode::Mode3D);
}

void PathDemo3DScene::Uninit()
{
    Scene::Uninit();
}
