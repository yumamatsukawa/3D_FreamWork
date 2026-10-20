#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Triggerデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 動かせる球・黄色い丸のトリガー(すり抜ける)・青い箱(ぶつかる)を置く。2D版は壁で囲み、3D版は地面の上に置く。
// camera: 3D版は、見る点のオブジェクト(CreateDemoStage3D の戻り値。球が動く向きの基準)。2D版は使わない
GameObject* CreateTriggerDemo(Scene& scene, DemoMode mode, GameObject* camera = nullptr);
