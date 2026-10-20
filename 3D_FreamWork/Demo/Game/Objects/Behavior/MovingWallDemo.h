#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// MovingWallデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 真ん中の壁(タグ "Wall")に、すぐ通れるドア(動く壁=Kinematic)と、遠回りの広いすき間がある。
// ドアが閉じると、PathFollowComponent で歩く緑の動く物は遠回りの道を通る(格子が自動で焼き直される)。
// 格子(NavGridComponent)は壁を置いた後に作る。操作役(ドアの開け閉め・焼き直しの設定・柵・表示)が付く
GameObject* CreateMovingWallDemo(Scene& scene, DemoMode mode);
