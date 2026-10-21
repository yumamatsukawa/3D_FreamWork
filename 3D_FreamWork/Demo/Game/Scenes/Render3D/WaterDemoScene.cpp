#include "WaterDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Render3D/DemoHills.h"
#include "../../Objects/Render3D/DemoWater.h"
#include "../../Objects/Render3D/Buoy.h"
#include "../../Objects/Render3D/FloatingBall.h"
#include "../../Objects/Render3D/WaterDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/WaterRenderer.h"

void WaterDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"水面のデモ(WaterRenderer)",
        L"広い海の水面です。丘の高い所だけが島になって出ています。色・波・水位を変えられます。",
        L"操作: 1・2 波の高さ / 3・4 波の速さ / 5 水の色 / ↑↓ 水位 / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: WaterRenderer(WaterSettings, GetSettings, GetHeightAt)" });
    CreateDemoSkybox(*this);
    CreateDemoHills(*this, { -2000.0f, 0.0f, -2000.0f });                          // 地形の中心が原点に来る位置(高さ0〜約500)
    // 水位300: 丘の面積の約13%(高さ300より高い所)が水面の上に出て、島になる。水面の大きさは5000四方(丘の外側もおおう)
    GameObject* water = CreateDemoWater(*this, { 0.0f, 300.0f, 0.0f });
    WaterRenderer* waterRenderer = water->GetComponent<WaterRenderer>();
    CreateBuoy(*this, waterRenderer, { 900.0f, 0.0f, -800.0f }, 160.0f);           // 島の南東の海に、大きい箱
    CreateFloatingBall(*this, waterRenderer, { 1400.0f, 700.0f, 100.0f }, 100.0f); // 島の東の海の上から、物理の球を落とす(浮力で浮く)
    CreateBuoy(*this, waterRenderer, { -150.0f, 0.0f, -1050.0f }, 60.0f);          // 島の南の海(カメラの近く)に、小さい箱
    CreateWaterDemo(*this, waterRenderer);
    CreateDemoCamera(*this, { 200.0f, 300.0f, 100.0f }, 0.0f, -30.0f, 2100.0f);    // 島の中心を、南の上空から見下ろす(島・海・箱が一緒に見える)
}

void WaterDemoScene::Uninit()
{
    Scene::Uninit();
}
