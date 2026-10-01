#pragma once

class Scene;
class GameObject;
class GameManager;
class EnemyManager;

// ゲーム全体の進行役(スコア・敵の出現・ゲームオーバー)を組み立てて、sceneに追加する。
// HUDなど他のオブジェクトが値を読めるよう、GameObjectではなくGameManager(Component)を返す
GameManager* CreateGameSystem(Scene& scene, EnemyManager* enemyManager, GameObject* player);
