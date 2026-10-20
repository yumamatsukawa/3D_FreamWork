#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Objectデモの中身(部品を付け外しする中央の物と、その操作役)を作る。mode: 2D版は板、3D版は立方体
GameObject* CreateObjectDemo(Scene& scene, DemoMode mode);
