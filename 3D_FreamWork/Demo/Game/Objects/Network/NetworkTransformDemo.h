#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// 位置の同期のデモの物を並べる。ホストが円を描いて動かす箱(Kinematic)と、それに押される球・丸(Dynamic)。
// ホスト(とオフライン)は物理で動かし、クライアントは受け取った位置を滑らかにつないで表示する。mode で2D版/3D版
GameObject* CreateNetworkTransformDemo(Scene& scene, DemoMode mode);