#include "TweenDemo3DScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGround.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Render/EaseRunner.h"

void TweenDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoSkybox(*this);
    CreateDemoGround(*this);
    CreateDemoGuide(*this, { L"補間デモ(Tween / Ease)",
        L"同じ時間で奥へ動く8個の立方体。位置・大きさ・向き・色が、同じ Ease で一緒に変わります。違うのは Ease だけです。",
        L"操作: SPACE で全部を最初からやり直す / 右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ / Esc: 戻る",
        L"使っているAPI: TweenComponent(MoveTo, ScaleTo, RotateTo, ColorTo, TweenValue, SetLoop), Ease::○○" });

    CreateEaseRunner(*this, L"Linear",     Ease::Linear,     0, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"InQuad",     Ease::InQuad,     1, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"OutQuad",    Ease::OutQuad,    2, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"InOutQuad",  Ease::InOutQuad,  3, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"OutCubic",   Ease::OutCubic,   4, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"OutBack",    Ease::OutBack,    5, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"OutElastic", Ease::OutElastic, 6, DemoMode::Mode3D);
    CreateEaseRunner(*this, L"OutBounce",  Ease::OutBounce,  7, DemoMode::Mode3D);
    CreateDemoCamera(*this, { 0.0f, 0.0f, 350.0f }, 0.0f, -35.0f, 1200.0f);
}

void TweenDemo3DScene::Uninit()
{
    Scene::Uninit();
}
