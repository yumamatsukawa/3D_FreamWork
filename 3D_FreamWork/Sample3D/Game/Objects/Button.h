#pragma once
#include <string>
#include <functional>
#include "../../../Engine/Transform.h"

class Scene;
class GameObject;

// ボタンを作る(label: 表示する文字、position: 位置、onClick: クリックされた時の処理)
GameObject* CreateButton(Scene& scene, const std::wstring& label, Transform tf,
    std::function<void()> onClick);
