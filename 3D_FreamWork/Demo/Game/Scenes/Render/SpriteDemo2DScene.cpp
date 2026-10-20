#include "SpriteDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/SpriteDemo.h"

void SpriteDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Spriteデモ(SpriteRenderer)",
        L"コマ・色・大きさ・回転を変えます。赤と青は、UI・2D・3Dの描き方で、前後と大きさが変わります。緑(UI)は必ず手前です。",
        L"操作: 1 コマ / 2 色 / 3 大きさ / 4 回転(押すたびに入り切り) / 5 UI・2D・3D / Tab: 3D版へ",
        L"使っているAPI: SpriteRenderer(SetSpriteIndex, SetColor, SetMode(SpriteMode)), Transform" });
    CreateSpriteDemo(*this, DemoMode::Mode2D);
}

void SpriteDemo2DScene::Uninit()
{
    Scene::Uninit();
}
