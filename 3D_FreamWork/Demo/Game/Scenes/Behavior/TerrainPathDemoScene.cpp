#include "TerrainPathDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Render3D/DemoHills.h"
#include "../../Objects/Behavior/TerrainPathDemo.h"

void TerrainPathDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"丘の経路探索のデモ(TerrainPath)",
        L"丘の高さ・坂を取り込んだ格子の上を、緑が歩きます。登れる坂の角度を変えると、急な坂(通れないマス)を避ける道が変わります。",
        L"操作: クリック 目的地 / 1・2・3 登れる坂(15・20・30度) / ↑↓ 速さ / F1 線 / R 戻す / 右ドラッグ 視点 / Q・E ズーム",
        L"使っているAPI: NavGridSettings(maxSlopeDegrees), NavGridComponent(SetTerrain), PathFollowComponent, GetHeightAt" });
    CreateDemoSkybox(*this);
    GameObject* hills = CreateDemoHills(*this, { -2000.0f, 0.0f, -2000.0f });   // 地形の中心が原点に来る位置
    CreateDemoCamera(*this, { 0.0f, 150.0f, 0.0f }, 0.0f, -50.0f, 2800.0f);
    CreateTerrainPathDemo(*this, hills);
}

void TerrainPathDemoScene::Uninit()
{
    Scene::Uninit();
}
