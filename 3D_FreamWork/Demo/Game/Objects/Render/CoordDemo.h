#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Coordデモの中身(座標の変換を見せる操作役と、奥行きの違うスプライト(2D)・調べられる立方体(3D))を作る
GameObject* CreateCoordDemo(Scene& scene, DemoMode mode);
