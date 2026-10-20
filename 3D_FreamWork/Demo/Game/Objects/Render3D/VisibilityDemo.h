#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Visibilityデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 2D版: 広い2Dの世界に、箱の画像を11x9個並べる。3D版: 地面に、球と立方体を9x9個並べる。どちらも操作役(画面に映っているかの判定・地図)が付く
// camera: 3D版は、見る点のオブジェクト(CreateDemoStage3D の戻り値。WASDで動かす)。2D版は使わない
GameObject* CreateVisibilityDemo(Scene& scene, DemoMode mode, GameObject* camera = nullptr);
