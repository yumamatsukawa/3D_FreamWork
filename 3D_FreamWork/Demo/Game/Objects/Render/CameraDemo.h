#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Cameraデモの中身(動かす自分と、動いたのが分かる目印、操作役)を作る。
// 2D版は自分を2Dカメラが追い、3D版は自分を三人称・一人称の3Dカメラで見る。3D版は、カメラを自分で動かすので、CreateDemoCamera は呼ばない(スカイボックスと地面だけ置く)
GameObject* CreateCameraDemo(Scene& scene, DemoMode mode);
