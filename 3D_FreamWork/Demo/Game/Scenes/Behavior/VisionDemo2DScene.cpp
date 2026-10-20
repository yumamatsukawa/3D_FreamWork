#include "VisionDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Behavior/VisionDemo.h"

void VisionDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"視野のデモ(Vision)",
        L"見張り役の視野(扇形)の中にターゲットがいて、壁にさえぎられていなければ見えます。見える/見えないで色が変わります。",
        L"操作: 矢印 ターゲット / A・D 向き / W・S 視野の角度 / 1・2 距離 / SPACE 回る / 5 壁 / F1 線 / R 戻す / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: VisionComponent(SetFieldOfView, SetRange, SetFacing, SetEyeOffset, SetObstacleTag, IsInCone, CanSee)" });
    CreateVisionDemo(*this, DemoMode::Mode2D, nullptr);
}

void VisionDemo2DScene::Uninit()
{
    Scene::Uninit();
}
