#include "TransformDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/TransformDemo.h"

void TransformDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Transformデモ(位置・回転・大きさ・親子)",
        L"大きい親を動かすと、小さい子がついてきます。親を回す・大きくすると、子も一緒に動きます。",
        L"操作: 矢印 親を動かす / R 親を回す / 1・2 親の大きさ / SPACE 親子をつなぐ・切る / Tab: 3D版へ",
        L"使っているAPI: Transform(position, rotate, scale), SetParent, GetWorldTransform" });
    CreateTransformDemo(*this, DemoMode::Mode2D);
}

void TransformDemo2DScene::Uninit()
{
    Scene::Uninit();
}
