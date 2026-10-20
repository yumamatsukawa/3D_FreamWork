#include "SpriteAnimDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/SpriteAnimDemo.h"

void SpriteAnimDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"SpriteAnimデモ(SpriteAnimationComponent)",
        L"1枚の画像に並んだコマを時間で送ります。ループするもの・1回だけで止まるもの・始まりをずらしたものを並べます。",
        L"操作: SPACE 再生・停止 / 1 0.5倍 / 2 通常 / 3 2倍 / 4 止める / Tab: 3D版へ",
        L"使っているAPI: SpriteAnimationComponent(AddClip, Play, Stop, SetSpeed, SetRandomStart)" });
    CreateSpriteAnimDemo(*this, DemoMode::Mode2D);
}

void SpriteAnimDemo2DScene::Uninit()
{
    Scene::Uninit();
}
