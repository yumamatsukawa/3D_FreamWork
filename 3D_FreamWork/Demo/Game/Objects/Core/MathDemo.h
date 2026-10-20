#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Mathデモの中身(MathUtilの結果を点と線で見せる役)を作る。mode: 2D版はマウスで、3D版はキーで動かす
GameObject* CreateMathDemo(Scene& scene, DemoMode mode);
