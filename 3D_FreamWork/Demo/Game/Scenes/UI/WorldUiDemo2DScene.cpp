#include "WorldUiDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/UI/WorldUiDemo.h"

void WorldUiDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"WorldUIデモ(物の頭上に出るUI)",
        L"敵の頭の上にHPゲージと名前を出します。敵が動いても、UIは敵についていきます。",
        L"操作: 矢印 主人公を動かす / 1・2・3 敵にダメージ / SPACE 全回復 / M 名前の大きさ / Tab: 3D版へ",
        L"使っているAPI: Image::WorldToScreen2D, GaugeRenderer(SetFollowTarget), TextRenderer(SetSizeMode), Image::DrawRect" });
    CreateWorldUiDemo(*this, DemoMode::Mode2D);
}

void WorldUiDemo2DScene::Uninit()
{
    Scene::Uninit();
}
