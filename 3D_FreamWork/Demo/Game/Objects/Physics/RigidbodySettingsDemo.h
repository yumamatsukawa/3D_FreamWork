#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// RigidbodySettingsデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 同じ箱を2つ(A=ふつう、B=設定を変えるもの)置く。2D版は壁で囲んだ中に2つのレーン、3D版は地面の上に手前と奥のレーン。操作役(設定の選択・蹴る)が付く
GameObject* CreateRigidbodySettingsDemo(Scene& scene, DemoMode mode);
