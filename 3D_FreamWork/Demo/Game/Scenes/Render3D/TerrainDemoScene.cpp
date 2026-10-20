#include "TerrainDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoCamera.h"
#include "../../Objects/Render3D/DemoHills.h"
#include "../../Objects/Render3D/TerrainDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"

void TerrainDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"地形のデモ(Terrain / TerrainRenderer)",
        L"高さの画像(heightmap.png)から作った丘です。黄色い玉の下の、地面の高さと向き(法線)を調べて表示します。",
        L"操作: 矢印 黄色い玉を動かす / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: Terrain::LoadFromImage, TerrainRenderer(GetHeightAt, GetNormalAt, Contains)" });
    CreateDemoSkybox(*this);
    GameObject* hills = CreateDemoHills(*this, { -2000.0f, 0.0f, -2000.0f });   // 地形の中心が原点に来る位置
    CreateTerrainDemo(*this, hills->GetComponent<TerrainRenderer>());
    CreateDemoCamera(*this, { 0.0f, 150.0f, 0.0f }, 0.0f, -30.0f, 2800.0f);
}

void TerrainDemoScene::Uninit()
{
    Scene::Uninit();
}
