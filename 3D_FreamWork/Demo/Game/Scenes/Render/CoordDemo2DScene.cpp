#include "CoordDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/CoordDemo.h"

void CoordDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Coordデモ(座標の変換)",
        L"マウスの画面の位置とワールドの位置を行き来します。右の3つは奥(z)だけ違いますが、zでは大きさも位置も変わりません。",
        L"操作: マウス 動かす / クリック 印を置く / R 印を消す / WASD・矢印 カメラを動かす / Tab: 3D版へ",
        L"使っているAPI: Image::WorldToScreen2D, ScreenToWorld2D, Input::GetMouseWorldPosition" });
    CreateCoordDemo(*this, DemoMode::Mode2D);
}

void CoordDemo2DScene::Uninit()
{
    Scene::Uninit();
}
