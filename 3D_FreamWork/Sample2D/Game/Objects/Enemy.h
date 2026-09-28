#pragma once

class Scene;
class GameObject;

// Enemyを構成するGameObjectを組み立てて、sceneに追加する
GameObject* CreateEnemy(Scene& scene);
