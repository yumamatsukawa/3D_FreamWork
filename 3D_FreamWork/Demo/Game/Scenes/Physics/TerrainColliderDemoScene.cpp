#include "TerrainColliderDemoScene.h"
#include "../../Objects/Common/DemoSkybox.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoWalker.h"
#include "../../Objects/Render3D/DemoHills.h"
#include "../../Objects/Physics/TerrainColliderDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"

void TerrainColliderDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"地形の当たり判定のデモ(TerrainRigidbodyComponent)",
        L"画像から作った丘を、箱で歩いて登ります。丘の見た目と同じデータの当たり判定なので、坂道をそのまま登れます。",
        L"操作: WASD 歩く(R でスタートへ) / 右ドラッグ 視点 / Q・E ズーム / Esc 戻る",
        L"使っているAPI: TerrainRigidbodyComponent, TerrainRenderer::GetHeightAt, RigidbodyComponent::SetVelocity" });
    CreateDemoSkybox(*this);
    GameObject* hills = CreateDemoHills(*this, { -2000.0f, 0.0f, -2000.0f });   // 地形の中心が原点に来る位置
    TerrainRenderer* terrain = hills->GetComponent<TerrainRenderer>();
    GameObject* walker = CreateDemoWalker(*this, terrain, { -1500.0f, 800.0f });
    CreateTerrainColliderDemo(*this, terrain, walker);
}

void TerrainColliderDemoScene::Uninit()
{
    Scene::Uninit();
}
