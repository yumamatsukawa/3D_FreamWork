#pragma once
#include "../../Components/Common/DemoGuideController.h"

class Scene;
class GameObject;

// 画面の右下に、2D/3Dの切り替えボタンを作る(メニューでは側を選ぶ。デモ本体では逆の側の版へ切り替える。文字と暗さは状態で変わる)
GameObject* CreateDemoModeButton(Scene& scene, DemoScreen screen);
