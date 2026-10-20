#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Homingデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 同じ目標(マウスに付いてくる目印)を、3つの動きで追いかける動く物を並べる:
// 青=ChaseComponent(一定の速さでまっすぐ)、橙=AccelerateChaseComponent(加速して行き過ぎる)、緑=HomingComponent(曲がる角度に限りがある)。
// 動く物は Kinematic(Transformを直接動かす。お互いにすり抜けるので、動きだけを比べられる)。操作役が付く
GameObject* CreateHomingDemo(Scene& scene, DemoMode mode);
