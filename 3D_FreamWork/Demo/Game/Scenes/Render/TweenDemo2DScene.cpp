#include "TweenDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render/EaseRunner.h"

void TweenDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"補間デモ(Tween / Ease)",
        L"同じ時間で右へ動く8個の丸。違うのは、動きの速さの変化(Ease)だけです。",
        L"操作: SPACE で全部を最初からやり直す(出だしの違いが見やすい) / Tab: 3D版へ / Esc: 戻る",
        L"使っているAPI: TweenComponent(MoveTo, ColorTo, TweenValue, SetLoop), Ease::○○" });

    CreateEaseRunner(*this, L"Linear",     Ease::Linear,     0, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"InQuad",     Ease::InQuad,     1, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"OutQuad",    Ease::OutQuad,    2, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"InOutQuad",  Ease::InOutQuad,  3, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"OutCubic",   Ease::OutCubic,   4, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"OutBack",    Ease::OutBack,    5, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"OutElastic", Ease::OutElastic, 6, DemoMode::Mode2D);
    CreateEaseRunner(*this, L"OutBounce",  Ease::OutBounce,  7, DemoMode::Mode2D);
}

void TweenDemo2DScene::Uninit()
{
    Scene::Uninit();
}
