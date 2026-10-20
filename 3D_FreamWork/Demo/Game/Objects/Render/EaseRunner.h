#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Animation/Ease.h"
#include <string>

class Scene;
class GameObject;

// 補間デモの1行(2D: 名前・線・左右に動く丸 / 3D: 名前・線・奥へ動く立方体)を作る(row: 何番目か。0から数える)
GameObject* CreateEaseRunner(Scene& scene, const std::wstring& easeName, EaseFunction ease, int row, DemoMode mode);
