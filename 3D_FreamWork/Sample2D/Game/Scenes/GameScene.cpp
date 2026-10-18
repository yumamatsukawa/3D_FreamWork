#include "GameScene.h"
#include "../Objects/Background.h"
#include "../Objects/Player.h"
#include "../Objects/Wall.h"
#include "../Objects/Guard.h"
#include "../Objects/GameSystem.h"
#include "../Objects/Navigation.h"
#include "../Objects/EnemySpawner.h"
#include "../Objects/BulletSpawner.h"
#include "../Objects/HUD.h"
#include "../Components/PlayerController.h"
#include "../../../Engine/Core/EventBus.h"
#include "../../../Engine/Core/GameObject.h"

void GameScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateBackground(*this);
    GameObject* player = CreatePlayer(*this);
    CreateWall(*this, { -250.0f, 150.0f }, { 40.0f, 300.0f });   // 見張り役とPlayerの間の壁
    CreateWall(*this, { 400.0f, -350.0f }, { 300.0f, 40.0f });
    CreateGuard(*this, player);
    GameManager* gameManager = CreateGameSystem(*this);
    NavGridComponent* navigation = CreateNavigation(*this);   // 壁をすべて置いた後に置く(敵が壁を避けるための格子)
    CreateEnemySpawner(*this, player, navigation);
    CreateBulletSpawner(*this);
    CreateHUD(*this, gameManager, player->GetComponent<PlayerController>());
}

void GameScene::Uninit()
{
    // 各ManagerがEventBusに登録した処理を消す(残すと、破棄済みのオブジェクトを触ってしまう)
    EventBus::Get().Clear();

    Scene::Uninit();
}
