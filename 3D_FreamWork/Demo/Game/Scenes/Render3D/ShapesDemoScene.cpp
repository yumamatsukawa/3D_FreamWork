#include "ShapesDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/ShapesDemo.h"

void ShapesDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"図形とマテリアルのデモ(Shapes)",
        L"手前は形(立方体・球・カプセル・OBJ)、奥の2列はマテリアル(見た目の設定)だけが違う球と立方体です。",
        L"操作: 1・2・3 列のオン/オフ / 4 形を回す / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: Mesh::CreateCube/CreateSphere/CreateCapsule/LoadOBJ, MeshRenderer, Material" });
    CreateDemoStage3D(*this, { 0.0f, 40.0f, 110.0f }, 0.0f, -32.0f, 1250.0f);
    CreateShapesDemo(*this);
}

void ShapesDemoScene::Uninit()
{
    Scene::Uninit();
}
