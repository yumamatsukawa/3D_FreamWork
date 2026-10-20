#pragma once

class Scene;
class GameObject;

// Shapesデモの本体を作る。3列の見本(手前=形、真ん中=球5個のマテリアル、奥=立方体5個のマテリアル)と、操作役
GameObject* CreateShapesDemo(Scene& scene);
