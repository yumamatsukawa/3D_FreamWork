#pragma once

class Scene;
class GameObject;
class EnemyManager;
class NavGridComponent;

// 敵の出現役を作る(出現した敵は、navigationの格子で壁と急な坂を避けながらtargetを追いかける)
EnemyManager* CreateEnemySpawner(Scene& scene, GameObject* target, NavGridComponent* navigation);
