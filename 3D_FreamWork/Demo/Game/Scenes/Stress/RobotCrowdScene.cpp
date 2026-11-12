#include "RobotCrowdScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Stress/RobotCrowd.h"

void RobotCrowdScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"負荷テストのデモ(RobotCrowd)",
        L"ランダムに歩き回るロボットを増やして、FPSと描画の数がどれくらい変わるかを測ります。物理のあり/なしも切り替えられます。",
        L"操作: 上下 か + - 50体ずつ増減 / P 物理 / 1 影 / 2 枠線 / 3 LOD / 4 画面外を省く / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: ModelRenderer, CapsuleRigidbodyComponent, RenderStats, Visibility, Lod, Shadow, Time::GetUnscaledDeltaTime" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 0.0f }, 0.0f, -50.0f, 2600.0f);
    CreateRobotCrowd(*this);
}

void RobotCrowdScene::Uninit()
{
    Scene::Uninit();
}
