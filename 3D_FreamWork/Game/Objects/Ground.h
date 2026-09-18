#pragma once

class Scene;
class GameObject;

// Groundを構成するGameObjectを組み立てて、sceneに追加する
GameObject* CreateGround(Scene& scene);
