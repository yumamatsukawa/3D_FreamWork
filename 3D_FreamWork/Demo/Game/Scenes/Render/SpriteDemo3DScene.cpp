#include "SpriteDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/SpriteDemo.h"

void SpriteDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Spriteデモ(SpriteRenderer)",
        L"3D空間に画像の板(ビルボード)を置きます。固定する軸(SetBillboardLock)によって、カメラを回した時の向きが変わります。",
        L"操作: 1 コマ / 2 色 / 3 大きさ / 4 回転(入り切り) / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: SpriteRenderer(SetWorldSpace, SetBillboardLock, SetSpriteIndex, SetColor)" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 300.0f }, 0.0f, -18.0f, 800.0f);
    CreateSpriteDemo(*this, DemoMode::Mode3D);
}

void SpriteDemo3DScene::Uninit()
{
    Scene::Uninit();
}
