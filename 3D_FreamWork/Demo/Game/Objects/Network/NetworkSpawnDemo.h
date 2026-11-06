#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// スポーン・破棄のデモの物を並べる。弾を撃つボタンと、弾を出す・消す管理役。mode で2D版/3D版
GameObject* CreateNetworkSpawnDemo(Scene& scene, DemoMode mode);