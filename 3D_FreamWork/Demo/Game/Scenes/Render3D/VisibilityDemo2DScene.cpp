#include "VisibilityDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/VisibilityDemo.h"

void VisibilityDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"画面に映っているかのデモ(Visibility)",
        L"広い世界に99個の箱が並んでいます。2Dカメラに映っている物だけを調べ、映っていない物は描かない設定にもできます。",
        L"操作: 1 描かない設定のオン/オフ / WASD・矢印 カメラを動かす / Tab: 3D版へ / Esc 戻る",
        L"使っているAPI: Visibility::IsCircleInView2D, IsRectInView2D, GetCulledCount, SpriteRenderer::SetFrustumCulling" });
    CreateVisibilityDemo(*this, DemoMode::Mode2D);
}

void VisibilityDemo2DScene::Uninit()
{
    Scene::Uninit();
}
