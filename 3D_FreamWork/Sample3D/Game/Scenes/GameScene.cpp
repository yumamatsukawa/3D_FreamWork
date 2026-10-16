#include "GameScene.h"
#include "../Objects/Skybox.h"
#include "../Objects/Sun.h"
#include "../Objects/Ground.h"
#include "../Objects/Hills.h"
#include "../Objects/ModelDisplay.h"
#include "../Objects/SkinDisplay.h"
#include "../Objects/Player.h"
#include "../Objects/Wall.h"
#include "../Objects/Guard.h"
#include "../Objects/GlassPanel.h"
#include "../Objects/Lantern.h"
#include "../Objects/GameSystem.h"
#include "../Objects/EnemySpawner.h"
#include "../Objects/BulletSpawner.h"
#include "../Objects/HUD.h"
#include "../Components/PlayerController.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/GameObject.h"

void GameScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateSkybox(*this);
    CreateSun(*this);   // 影を有効にする(影を落とす・受ける物は、各オブジェクトの Material で決める)
    CreateGround(*this);
    CreateHills(*this, { 1500.0f, -50.0f, -2000.0f });   // 遠くの起伏のある地形(東の方。当たり判定は6.8で付ける)
    CreateModelDisplay(*this);
    CreateSkinDisplay(*this);
    GameObject* player = CreatePlayer(*this);
    CreateWall(*this, { -400.0f, 50.0f, 300.0f }, { 40.0f, 200.0f, 300.0f });   // 見張り役とPlayerの間の壁
    CreateWall(*this, { 500.0f, 50.0f, -500.0f }, { 300.0f, 200.0f, 40.0f });
    CreateGuard(*this, player);
    CreateGlassPanel(*this, { 400.0f, 50.0f, 300.0f }, { 300.0f, 200.0f, 10.0f });   // 半透明で光沢のある板
    CreateLantern(*this, { 0.0f, 0.0f, 0.0f });   // 周りを照らしながら回るランタン(点光源)
    GameManager* gameManager = CreateGameSystem(*this);
    CreateEnemySpawner(*this, player);
    CreateBulletSpawner(*this);
    CreateHUD(*this, gameManager, player->GetComponent<PlayerController>());
}

void GameScene::Uninit()
{
    // 各ManagerがEventBusに登録した処理を消す(残すと、破棄済みのオブジェクトを触ってしまう)
    EventBus::Get().Clear();

    Scene::Uninit();
}
