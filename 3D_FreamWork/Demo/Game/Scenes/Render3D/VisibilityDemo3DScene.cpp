#include "VisibilityDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/VisibilityDemo.h"

void VisibilityDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"画面に映っているかのデモ(Visibility)",
        L"地面に81個の球と立方体が並んでいます。3Dカメラに映っている物だけを調べ、映っていない物は描かない設定にもできます。",
        L"操作: 1 描かない設定のオン/オフ / WASD・矢印 見る点を動かす / 右ドラッグ 向き / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: Visibility::IsSphereInView3D, IsBoxInView3D, GetCulledCount, MeshRenderer::SetFrustumCulling" });
    GameObject* camera = CreateDemoStage3D(*this, { 0.0f, 50.0f, 0.0f }, 0.0f, -35.0f, 800.0f);
    CreateVisibilityDemo(*this, DemoMode::Mode3D, camera);
}

void VisibilityDemo3DScene::Uninit()
{
    Scene::Uninit();
}
