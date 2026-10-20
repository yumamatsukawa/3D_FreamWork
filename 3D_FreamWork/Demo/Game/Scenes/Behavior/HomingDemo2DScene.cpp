#include "HomingDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/HomingDemo.h"

void HomingDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"追いかけ方を比べるデモ(Homing)",
        L"同じ目標を、青(一定の速さ)・橙(加速して行き過ぎる)・緑(曲がる角度に限りがある)の3つが追います。通った道が残ります。",
        L"操作: マウス 目標 / 1・2・3 入り切り / ↑↓ 緑の曲がる速さ / ←→ 橙の加速 / R 戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: ChaseComponent, AccelerateChaseComponent, HomingComponent(SetTurnSpeed, SetFaceMovement)" });
    CreateHomingDemo(*this, DemoMode::Mode2D);
}

void HomingDemo2DScene::Uninit()
{
    Scene::Uninit();
}
