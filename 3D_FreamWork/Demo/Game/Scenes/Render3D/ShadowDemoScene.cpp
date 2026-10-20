#include "ShadowDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGround.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Common/DemoLights.h"
#include "../../Objects/Render3D/ShadowDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/LightComponent.h"

void ShadowDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"影のデモ(Shadow)",
        L"太陽の向きで影が動きます。影を落とす・受けるは、物ごとのマテリアルで決めます。",
        L"操作: 1〜5 切り替え / 矢印 太陽を手で動かす・SPACE 自動 / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: Shadow::GetSettings, Material(castShadow, receiveShadow), SetCastShadow" });
    CreateDemoSkybox(*this);
    CreateDemoGround(*this);
    GameObject* spot = CreateDemoSpotLight(*this, { 100.0f, 500.0f, -300.0f }, true);
    CreateShadowDemo(*this, spot->GetComponent<LightComponent>());
    CreateDemoCamera(*this, { 50.0f, 60.0f, 100.0f }, 0.0f, -25.0f, 1000.0f);
}

void ShadowDemoScene::Uninit()
{
    Scene::Uninit();
}
