#include "SpriteAnimDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/SpriteAnimDemo.h"

void SpriteAnimDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"SpriteAnimデモ(SpriteAnimationComponent)",
        L"3D空間に置いたビルボードのスプライトで、同じコマ送りを再生します。視点を回しても、板はカメラの方を向きます。",
        L"操作: SPACE 再生・停止 / 1 0.5倍 / 2 通常 / 3 2倍 / 4 止める / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: SpriteAnimationComponent(AddClip, Play, Stop, SetSpeed, SetRandomStart)" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 380.0f }, 0.0f, -18.0f, 850.0f);
    CreateSpriteAnimDemo(*this, DemoMode::Mode3D);
}

void SpriteAnimDemo3DScene::Uninit()
{
    Scene::Uninit();
}
