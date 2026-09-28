#pragma once

class Scene;
class GameObject;

// Playerを構成するGameObjectを組み立てて、sceneに追加する
GameObject* CreatePlayer(Scene& scene);
