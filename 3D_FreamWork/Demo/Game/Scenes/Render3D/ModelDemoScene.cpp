#include "ModelDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/ModelDemo.h"

void ModelDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"FBXモデルのデモ(Model)",
        L"2つのパーツ(Base と Top)でできたFBXを置きます。パーツを名前で探して動かすと、そのパーツと子のパーツだけが動きます。",
        L"操作: 1〜3 Topの回転・上下・大きさ / 4 全体を回す / 5 全体の大きさ / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: ModelRenderer, GetNodeTransform, Model(GetNodeCount, GetNode, GetBoundingRadius)" });
    CreateDemoStage3D(*this, { -250.0f, 60.0f, 100.0f }, 0.0f, -20.0f, 900.0f);
    CreateModelDemo(*this);
}

void ModelDemoScene::Uninit()
{
    Scene::Uninit();
}
