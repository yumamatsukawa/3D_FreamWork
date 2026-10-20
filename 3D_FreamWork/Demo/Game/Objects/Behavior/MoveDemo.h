#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Moveデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 壁で囲んだ競技場に、壁を1枚と、ChaseComponent を付けた動く物2つ(青=マウスを追う / 橙=クリックした場所へ向かう)を置く。
// 操作役(速さ・止まる距離・目印の移動・表示)が付く
GameObject* CreateMoveDemo(Scene& scene, DemoMode mode);
