#pragma once

class Scene;
class GameObject;

// Cubeを構成するGameObjectを組み立てて、sceneに追加する
GameObject* CreateCube(Scene& scene);
