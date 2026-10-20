#pragma once
#include "../../Components/Render/TransitionDemoController.h"

class Scene;
class GameObject;

// Transitionデモの中身(演出を選ぶ操作役と、切り替え中に時間が止まるのを見せる回る箱)を作る。page: 2つのシーンのどちらか(Bは3Dの箱)
GameObject* CreateTransitionDemo(Scene& scene, TransitionPage page);
