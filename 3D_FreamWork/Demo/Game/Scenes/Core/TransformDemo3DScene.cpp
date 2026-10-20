#include "TransformDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/TransformDemo.h"

void TransformDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Transformデモ(位置・回転・大きさ・親子)",
        L"大きい親を動かすと、小さい子がついてきます。3つの軸で回すと、子は親の周りを回ります。",
        L"操作: 矢印 親を動かす / R・T 親を回す / 1・2 親の大きさ / SPACE 親子をつなぐ・切る / 右ドラッグ 視点 / Tab: 2D版へ",
        L"使っているAPI: Transform(position, rotate, scale), SetParent, GetWorldMatrix" });
    CreateDemoStage3D(*this, { 0.0f, 0.0f, 300.0f }, 0.0f, -25.0f, 800.0f);
    CreateTransformDemo(*this, DemoMode::Mode3D);
}

void TransformDemo3DScene::Uninit()
{
    Scene::Uninit();
}
