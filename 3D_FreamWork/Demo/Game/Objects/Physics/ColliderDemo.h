#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Colliderデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 2D版: 壁で囲んだ中に、固定の見本(円・四角・カプセル)と左右に往復する台を置く。
// 3D版: 地面の上に、固定の見本(球・箱・カプセル)と左右に往復する台を置く。どちらも、操作役(クリックで物を出す・枠線)が付く
GameObject* CreateColliderDemo(Scene& scene, DemoMode mode);
