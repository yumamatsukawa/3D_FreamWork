#include "MovingWallDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/MovingWallDemo.h"

void MovingWallDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"動く壁のデモ(MovingWall)",
        L"ドア(動く壁)が閉じると、緑は遠回りの道を通ります。格子の焼き直しと、物理の無い柵(SetBlockedBox)で経路が変わります。",
        L"操作: 1 ドア / 2 クリックの意味 / 3 自動焼き直し / 4 今焼く / C 柵を消す / F1 線 / R 戻す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: NavGridComponent(SetAutoRebakeInterval, RequestRebake), NavGrid(SetBlockedBox, GetVersion)" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 60.0f }, 0.0f, -58.0f, 1250.0f);
    CreateMovingWallDemo(*this, DemoMode::Mode3D);
}

void MovingWallDemo3DScene::Uninit()
{
    Scene::Uninit();
}
