#include "PathDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/PathDemo.h"

void PathDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"経路探索のデモ(Path)",
        L"緑は格子の上で経路を探して壁を避け、赤は目的地へまっすぐ向かうだけで壁に止まります。クリックで目的地を動かします。",
        L"操作: クリック 目的地 / ↑↓ 速さ / ←→ 探し直す間隔 / F1 格子と経路の線 / R 戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: NavGridComponent, PathFollowComponent(SetNavigation, SetRepathInterval, HasPath), SetDebugDrawEnabled" });
    CreatePathDemo(*this, DemoMode::Mode2D);
}

void PathDemo2DScene::Uninit()
{
    Scene::Uninit();
}
