#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Pathデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 壁(タグ "Wall")で、行き止まりのように見える通り道を作って、同じ目的地(クリックで動く目印)へ向かう2つの動く物を置く:
// 緑=PathFollowComponent(格子の上で経路を探して、壁を避ける)、赤=ChaseComponent(まっすぐ向かうだけで、壁にぶつかって止まる)。
// 格子(NavGridComponent)は壁を置いた後に作る。操作役(目印の移動・設定・表示)が付く
GameObject* CreatePathDemo(Scene& scene, DemoMode mode);
