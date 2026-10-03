#pragma once

class Scene;
class GameObject;

// Player(立方体。カメラ基準のWASDで移動・カメラと同じ向きを向く・左クリックで発射)を作る
GameObject* CreatePlayer(Scene& scene);
