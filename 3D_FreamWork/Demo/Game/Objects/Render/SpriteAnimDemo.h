#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// SpriteAnimデモの中身(player.png の4つのクリップと、始まりをずらした5つのスプライト、操作役)を作る。mode: 2D版は画面に並べて、3D版はビルボードで置く
GameObject* CreateSpriteAnimDemo(Scene& scene, DemoMode mode);
