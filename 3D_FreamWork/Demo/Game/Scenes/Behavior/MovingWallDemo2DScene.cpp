#include "MovingWallDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/MovingWallDemo.h"

void MovingWallDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"動く壁のデモ(MovingWall)",
        L"ドア(動く壁)が閉じると、緑は遠回りの道を通ります。格子の焼き直しと、物理の無い柵(SetBlockedBox)で経路が変わります。",
        L"操作: 1 ドア / 2 クリックの意味 / 3 自動焼き直し / 4 今焼く / C 柵を消す / F1 線 / R 戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: NavGridComponent(SetAutoRebakeInterval, RequestRebake), NavGrid(SetBlockedBox, GetVersion)" });
    CreateMovingWallDemo(*this, DemoMode::Mode2D);
}

void MovingWallDemo2DScene::Uninit()
{
    Scene::Uninit();
}
