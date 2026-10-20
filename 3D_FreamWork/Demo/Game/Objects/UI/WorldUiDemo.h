#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// WorldUIデモの中身(主人公・頭上にHPゲージと名前が出る敵3体・操作役)を作る。mode: 2D版は板、3D版は立方体
GameObject* CreateWorldUiDemo(Scene& scene, DemoMode mode);
