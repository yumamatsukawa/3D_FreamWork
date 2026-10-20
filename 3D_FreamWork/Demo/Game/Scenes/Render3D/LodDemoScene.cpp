#include "LodDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/LodDemo.h"

void LodDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"距離で省くデモ(LOD)",
        L"225本の柱のうち、カメラから遠い柱は描かず・影を落とさず・枠線を描きません。省いた分だけ、描画命令と三角形が減ります。",
        L"操作: 1 LODのオン/オフ / 上下 距離の倍率 / 2 影 / 3 枠線 / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: Lod::SetEnabled, SetGlobalScale, MeshRenderer::SetLod, RenderStats" });
    CreateDemoStage3D(*this, { 0.0f, 100.0f, 0.0f }, 0.0f, -25.0f, 2000.0f);
    CreateLodDemo(*this);
}

void LodDemoScene::Uninit()
{
    Scene::Uninit();
}
