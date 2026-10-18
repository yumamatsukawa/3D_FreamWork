#include "EnemySpawner.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../Components/EnemyManager.h"

EnemyManager* CreateEnemySpawner(Scene& scene, GameObject* target, NavGridComponent* navigation) {
    GameObject* obj = scene.CreateObject("EnemySpawner");
    // 敵の出現と、追いかける相手・壁を避けるための格子
    EnemyManager* enemyManager = obj->AddComponent<EnemyManager>();
    enemyManager->Setup(target, navigation);
    return enemyManager;
}
