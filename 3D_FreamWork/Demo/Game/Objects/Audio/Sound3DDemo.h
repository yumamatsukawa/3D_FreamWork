#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Sound3Dデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// ぐるぐる回って音を出す物(橙。AudioSourceComponent)と、WASDで動かす聞く側(青。AudioListenerComponent)を置く。
// cameraObject: 3D版の3Dカメラの物(CreateDemoStage3D の戻り値。WASDの向きの基準)。2D版は nullptr。操作役が付く
GameObject* CreateSound3DDemo(Scene& scene, DemoMode mode, GameObject* cameraObject = nullptr);
