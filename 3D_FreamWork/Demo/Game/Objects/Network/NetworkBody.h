#pragma once
#include "../Common/DemoBody.h"
#include "../../Scenes/Common/DemoRegistry.h"

// 位置をホストからクライアントへ同期する、剛体付きの物を作る(CreateDemoBody2D / 3D に NetworkId と位置の同期を足したもの)。
// ホストの物は desc.type のまま(Dynamicなら物理で動く)、クライアントのコピーはDynamicをKinematicにする
// (クライアントでは Transform を書き換えて動かすので、Dynamicのままにしない)。sceneId は 1〜999 で、ホストとクライアントで同じ番号にする
GameObject* CreateNetworkBody(Scene& scene, DemoMode mode, int sceneId, DemoBodyDesc desc);