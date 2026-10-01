#pragma once

class Scene;
class GameObject;
class GameManager;
class PlayerController;

// スコア・HP・GAME OVERの画面表示を作る(表示する値の持ち主を渡す)
GameObject* CreateHUD(Scene& scene, GameManager* gameManager, PlayerController* player);
