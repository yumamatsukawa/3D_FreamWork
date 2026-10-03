#include "GameScene.h"
#include "../Objects/Skybox.h"
#include "../Objects/Ground.h"
#include "../Objects/Player.h"
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
    CreateGround(*this);
    GameObject* player = CreatePlayer(*this);
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
