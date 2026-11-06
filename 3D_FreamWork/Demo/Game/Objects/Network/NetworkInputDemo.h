#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// 入力の同期のデモの物を並べる。プレイヤー0〜3それぞれの箱(Dynamic)。自分の番号の箱を、キー入力で動かす。
// 入力はホストへ送られ、ホストが箱を動かし、結果が全員に同期される。mode で2D版/3D版
void CreateNetworkInputDemo(Scene& scene, DemoMode mode);