#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Transformデモの中身(大きい親・親についてくる小さい子・操作役)を作る。mode: 2D版は板、3D版は立方体
GameObject* CreateTransformDemo(Scene& scene, DemoMode mode);
