#include "GameSystem.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/GameManager.h"

GameManager* CreateGameSystem(Scene& scene, EnemyManager* enemyManager, GameObject* player) {
    GameObject* obj = scene.CreateObject("GameSystem");
    GameManager* gameManager = obj->AddComponent<GameManager>();
    gameManager->Setup(enemyManager, player);
    return gameManager;
}
