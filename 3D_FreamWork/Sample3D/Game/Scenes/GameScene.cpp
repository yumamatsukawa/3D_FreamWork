#include "GameScene.h"
#include "../Objects/Skybox.h"
#include "../Objects/Sun.h"
#include "../Objects/Ground.h"
#include "../Objects/Hills.h"
#include "../Objects/Water.h"
#include "../Objects/ModelDisplay.h"
#include "../Objects/SkinDisplay.h"
#include "../Objects/Player.h"
#include "../Objects/Wall.h"
#include "../Objects/WallField.h"
#include "../Objects/Guard.h"
#include "../Objects/GlassPanel.h"
#include "../Objects/Lantern.h"
#include "../Objects/GameSystem.h"
#include "../Objects/Navigation.h"
#include "../Objects/EnemySpawner.h"
#include "../Objects/BulletSpawner.h"
#include "../Objects/HUD.h"
#include "../Components/PlayerController.h"
#include "../../../Engine/Core/EventBus.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render3D/Lod.h"

namespace {
    // 確認用: false にすると、柱の距離で省く設定(LOD)を全部無視する。HUDの Draw / Tri の数を、true と false で比べる
    constexpr bool lodEnabled = true;
}

void GameScene::Init()
{
    Lod::SetEnabled(lodEnabled);

    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateSkybox(*this);
    CreateSun(*this);   // 影を有効にする(影を落とす・受ける物は、各オブジェクトの Material で決める)
    CreateGround(*this);
    GameObject* hills = CreateHills(*this, { 1500.0f, -50.0f, -2000.0f });   // 遠くの起伏のある地形(東の方。歩ける)
    CreateWater(*this, { 2075.0f, -16.0f, 1350.0f });   // 丘の谷(北西のくぼみ)に張った池(波打つ半透明の水面。通り抜けられる)
    CreateModelDisplay(*this);
    CreateSkinDisplay(*this);
    GameObject* player = CreatePlayer(*this);
    CreateWall(*this, { -400.0f, 50.0f, 300.0f }, { 40.0f, 200.0f, 300.0f });   // 見張り役とPlayerの間の壁
    CreateWall(*this, { 500.0f, 50.0f, -500.0f }, { 300.0f, 200.0f, 40.0f });
    CreateWallField(*this, { 0.0f, 0.0f, -6000.0f }, 15);   // 南の遠くに柱を15x15本(経路探索の格子の外。LODと描画カウンタを測る場所)
    CreateGuard(*this, player);
    CreateGlassPanel(*this, { 400.0f, 50.0f, 300.0f }, { 300.0f, 200.0f, 10.0f });   // 半透明で光沢のある板
    CreateLantern(*this, { 0.0f, 0.0f, 0.0f });   // 周りを照らしながら回るランタン(点光源)
    GameManager* gameManager = CreateGameSystem(*this);
    NavGridComponent* navigation = CreateNavigation(*this, hills);   // 壁と丘をすべて置いた後に置く(敵が壁と急な坂を避けるための格子)
    CreateEnemySpawner(*this, player, navigation);
    CreateBulletSpawner(*this);
    CreateHUD(*this, gameManager, player->GetComponent<PlayerController>());
}

void GameScene::Uninit()
{
    // 各ManagerがEventBusに登録した処理を消す(残すと、破棄済みのオブジェクトを触ってしまう)
    EventBus::Get().Clear();
    Lod::SetEnabled(true);   // 確認用の切り替えを元に戻す

    Scene::Uninit();
}
