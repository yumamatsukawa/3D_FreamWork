#pragma once

class Scene;
class GameObject;

// Player(WASDで移動・マウスの方を向く・左クリックで発射)を作る
GameObject* CreatePlayer(Scene& scene);
