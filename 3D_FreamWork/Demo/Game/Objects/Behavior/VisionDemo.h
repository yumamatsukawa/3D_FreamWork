#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Visionデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 壁(タグ "Wall")で囲んだ競技場に、VisionComponent を付けた見張り役(動かない)・動かせるターゲット・途中の壁を置く。
// 操作役(視野の設定・ターゲットの移動・表示)が付く。camera: 3D版だけ、見る向きの基準にするカメラのオブジェクト(2D版は nullptr)
GameObject* CreateVisionDemo(Scene& scene, DemoMode mode, GameObject* camera);
