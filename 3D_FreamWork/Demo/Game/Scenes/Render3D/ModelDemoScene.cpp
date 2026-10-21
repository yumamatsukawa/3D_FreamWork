#include "ModelDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/ModelDemo.h"

void ModelDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"FBXモデルのデモ(Model)",
        L"FBXの2パーツ(Base と Top)を名前で探して動かすと、そのパーツと子だけ動きます。左のロボットと奥の箱は別のFBXです。",
        L"操作: 1〜3 Top / 4 全体を回す / 5 全体の大きさ / 6 ロボット表示 / 7 ロボットを染める / 8 FBXの色 / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: ModelRenderer(GetNodeTransform, SetColor, SetUseModelColors), Model(GetNode, GetNodeColor, GetBoundingRadius)" });
    CreateDemoStage3D(*this, { -250.0f, 60.0f, 100.0f }, 0.0f, -20.0f, 900.0f);
    CreateModelDemo(*this);
}

void ModelDemoScene::Uninit()
{
    Scene::Uninit();
}
