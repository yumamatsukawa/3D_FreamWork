#include "WorldTextDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/UI/WorldTextDemo.h"

void WorldTextDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"WorldTextデモ(3Dの世界に貼る文字)",
        L"看板の文字を、壁に貼り付けたり、傾けたり、いつもカメラの方へ向けたりします。柱の後ろには隠れ、遠いほど小さく見えます。",
        L"操作: B ビルボード / D 両面 / 左右 板を回す / C カウント / 1 同じ文字・違う文字 / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: WorldTextRenderer(SetText, SetStyle, SetBillboard, SetDoubleSided, SetBackground, GetSharedTextureCount)" });
    CreateDemoStage3D(*this, { 0.0f, 60.0f, 200.0f }, 0.0f, -12.0f, 1100.0f);
    CreateWorldTextDemo(*this);
}

void WorldTextDemoScene::Uninit()
{
    Scene::Uninit();
}
