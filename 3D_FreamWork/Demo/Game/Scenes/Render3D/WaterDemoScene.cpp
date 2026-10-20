#include "WaterDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Render3D/DemoHills.h"
#include "../../Objects/Render3D/DemoWater.h"
#include "../../Objects/Render3D/Buoy.h"
#include "../../Objects/Render3D/WaterDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/WaterRenderer.h"

void WaterDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"水面のデモ(WaterRenderer)",
        L"丘の谷にある池の水面です。水の色・波の高さ・波の速さを変えられます。赤い箱は波に浮かびます。",
        L"操作: 1・2 波の高さ / 3・4 波の速さ / 5 水の色 / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: WaterRenderer(WaterSettings, GetSettings, GetHeightAt)" });
    CreateDemoSkybox(*this);
    CreateDemoHills(*this, { -2000.0f, 0.0f, -2000.0f });                          // 地形の中心が原点に来る位置(池のくぼみになる)
    GameObject* water = CreateDemoWater(*this, { -1425.0f, 34.0f, 1350.0f });      // 丘の谷(北西のくぼみ)の池
    WaterRenderer* waterRenderer = water->GetComponent<WaterRenderer>();
    CreateBuoy(*this, waterRenderer, { -1425.0f, 0.0f, 1350.0f });
    CreateWaterDemo(*this, waterRenderer);
    CreateDemoCamera(*this, { -1425.0f, 30.0f, 1350.0f }, 0.0f, -45.0f, 900.0f);
}

void WaterDemoScene::Uninit()
{
    Scene::Uninit();
}
