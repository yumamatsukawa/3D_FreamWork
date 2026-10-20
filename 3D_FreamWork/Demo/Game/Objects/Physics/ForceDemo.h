#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Forceデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 横に並んだ2本のレーンに、軽い箱と重い箱を置き、両端に壁を立てる。操作役(力の種類・SPACEで押す)が付く
GameObject* CreateForceDemo(Scene& scene, DemoMode mode);
