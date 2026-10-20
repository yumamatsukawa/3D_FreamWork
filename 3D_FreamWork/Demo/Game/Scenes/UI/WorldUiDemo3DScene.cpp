#include "WorldUiDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/UI/WorldUiDemo.h"

void WorldUiDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"WorldUIデモ(物の頭上に出るUI)",
        L"3Dの敵の頭の上にHPゲージと名前を出します。視点を回しても、UIは敵についていき、カメラの後ろでは出ません。",
        L"操作: 矢印 主人公を動かす / 1・2・3 敵にダメージ / SPACE 全回復 / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: Image::WorldToScreen3D(z > 0), GaugeRenderer(SetUse3DCamera), TextRenderer, Image::DrawRect" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 320.0f }, 0.0f, -22.0f, 850.0f);
    CreateWorldUiDemo(*this, DemoMode::Mode3D);
}

void WorldUiDemo3DScene::Uninit()
{
    Scene::Uninit();
}
