#pragma once

class Scene;
class GameObject;
class EnemyManager;

// 敵の出現役を作る(出現した敵はtargetを追いかける)
EnemyManager* CreateEnemySpawner(Scene& scene, GameObject* target);
