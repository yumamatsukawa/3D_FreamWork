#pragma once

class Scene;
class GameObject;
class GameManager;
class PlayerController;

// 画面のUI(スコア・HP・ゲームオーバー表示)を組み立てて、sceneに追加する。
// 表示する値の持ち主(gameManager/player)を引数で受け取り、HUDControllerに渡す
GameObject* CreateHUD(Scene& scene, GameManager* gameManager, PlayerController* player);
