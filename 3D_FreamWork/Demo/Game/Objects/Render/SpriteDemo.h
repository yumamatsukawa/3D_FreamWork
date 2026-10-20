#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Spriteデモの中身(見た目を変えるスプライトと、操作役)を作る。mode: 2D版は並べたスプライトと重ねた2つ、3D版は立方体の周りのビルボード
GameObject* CreateSpriteDemo(Scene& scene, DemoMode mode);
