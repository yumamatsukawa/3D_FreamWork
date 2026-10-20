#include "VisionDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/VisionDemo.h"

void VisionDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"視野のデモ(Vision)",
        L"見張り役の視野(扇形)の中にターゲットがいて、壁にさえぎられていなければ見えます。見える/見えないで色が変わります。",
        L"操作: 矢印 ターゲット / A・D 向き / W・S 角度 / 1・2 距離 / 4 目の高さ / 5 壁 / SPACE 回る / F1 線 / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: VisionComponent(SetFieldOfView, SetRange, SetFacing, SetEyeOffset, SetObstacleTag, IsInCone, CanSee)" });
    GameObject* camera = CreateDemoStage3D(*this, { 0.0f, 0.0f, 60.0f }, 0.0f, -58.0f, 1250.0f);
    CreateVisionDemo(*this, DemoMode::Mode3D, camera);
}

void VisionDemo3DScene::Uninit()
{
    Scene::Uninit();
}
