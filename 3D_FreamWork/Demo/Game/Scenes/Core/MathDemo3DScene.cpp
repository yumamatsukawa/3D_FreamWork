#include "MathDemo3DScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/MathDemo.h"

void MathDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"Mathデモ(MathUtil)",
        L"3Dのベクトルの計算(内積・外積・Normalize)と、Lerp・MoveTowards を、点と線で見ます。",
        L"操作: 1〜4 関数を選ぶ / 3: 矢印・W・S ベクトルを動かす / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: Math::Lerp, Dot, Cross, Normalize, Length, Distance, MoveTowards" });
    CreateDemoStage3D(*this, { 0.0f, 60.0f, 300.0f }, 0.0f, -20.0f, 750.0f);
    CreateMathDemo(*this, DemoMode::Mode3D);
}

void MathDemo3DScene::Uninit()
{
    Scene::Uninit();
}
