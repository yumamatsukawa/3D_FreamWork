#include "MathDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/MathDemo.h"

void MathDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Mathデモ(MathUtil)",
        L"Lerp・Clamp・角度・距離・乱数の結果を、点と線で見ます。マウスで値を動かします。",
        L"操作: 1〜4 関数を選ぶ / マウス 値を動かす / 4: SPACE サイコロ・クリック 数え直す / Tab: 3D版へ",
        L"使っているAPI: Math::Lerp, Clamp, Normalize, AngleToTarget2D, MoveTowardsAngle, RandomRange, RandomOnCircle" });
    CreateMathDemo(*this, DemoMode::Mode2D);
}

void MathDemo2DScene::Uninit()
{
    Scene::Uninit();
}
