#include "LightDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGround.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Common/DemoLights.h"
#include "../../Objects/Render3D/LightTargets.h"
#include "../../Objects/Render3D/LightDemo.h"

void LightDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"ライトのデモ(Light)",
        L"同じ白い球5個と立方体5個に、太陽(平行光源)・点光源・スポットライトを当てて、光の違いを見比べます。",
        L"操作: 1・2・3 光のオン/オフ / 矢印 太陽の向き / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: Image::GetLight, LightComponent(Point, Spot, SetSpotAngle)" });
    CreateDemoSkybox(*this);
    CreateDemoGround(*this);
    CreateLightTargets(*this);
    GameObject* point = CreateDemoPointLight(*this, { 0.0f, 80.0f, 120.0f });
    GameObject* spot = CreateDemoSpotLight(*this, { 0.0f, 500.0f, -300.0f }, false);
    CreateLightDemo(*this, point, spot);
    CreateDemoCamera(*this, { 0.0f, 60.0f, 150.0f }, 0.0f, -15.0f, 900.0f);
}

void LightDemoScene::Uninit()
{
    Scene::Uninit();
}
