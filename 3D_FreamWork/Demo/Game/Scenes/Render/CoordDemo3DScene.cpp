#include "CoordDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/CoordDemo.h"

void CoordDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Coordデモ(座標の変換)",
        L"3Dの物の頭の上に文字を出します。マウスの位置から線を伸ばして、マウスの下にある物をRaycastで探します。",
        L"操作: マウス 動かす / クリック 物を選ぶ / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: Image::WorldToScreen3D(z > 0), ScreenPointToRay3D, Physics::Raycast" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 480.0f }, 0.0f, -22.0f, 1100.0f);
    CreateCoordDemo(*this, DemoMode::Mode3D);
}

void CoordDemo3DScene::Uninit()
{
    Scene::Uninit();
}
