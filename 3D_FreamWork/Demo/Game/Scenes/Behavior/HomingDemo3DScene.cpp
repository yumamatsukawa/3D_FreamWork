#include "HomingDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/HomingDemo.h"

void HomingDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"追いかけ方を比べるデモ(Homing)",
        L"同じ目標を、青(一定の速さ)・橙(加速して行き過ぎる)・緑(曲がる角度に限りがある)の3つが追います。通った道が残ります。",
        L"操作: マウス 目標 / 1・2・3 入り切り / ↑↓ 緑の曲がる速さ / ←→ 橙の加速 / R 戻す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: ChaseComponent, AccelerateChaseComponent, HomingComponent(SetTurnSpeed, SetFaceMovement)" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 60.0f }, 0.0f, -58.0f, 1250.0f);
    CreateHomingDemo(*this, DemoMode::Mode3D);
}

void HomingDemo3DScene::Uninit()
{
    Scene::Uninit();
}
