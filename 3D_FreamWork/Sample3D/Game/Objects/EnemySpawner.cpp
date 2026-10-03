#include "EnemySpawner.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/EnemyManager.h"

EnemyManager* CreateEnemySpawner(Scene& scene, GameObject* target) {
    GameObject* obj = scene.CreateObject("EnemySpawner");
    // 敵の出現と、追いかける相手
    EnemyManager* enemyManager = obj->AddComponent<EnemyManager>();
    enemyManager->Setup(target);
    return enemyManager;
}
