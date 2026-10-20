#pragma once
#include <string>
#include <functional>
#include <DirectXMath.h>

class Scene;
class GameObject;

// 大きさを決められるボタンを作る(label: 表示する文字、position: 中心の位置、size: 幅と高さ、textSize: 文字の大きさ、onClick: クリックされた時の処理)
GameObject* CreateDemoButton(Scene& scene, const std::wstring& label, DirectX::XMFLOAT2 position,
    DirectX::XMFLOAT2 size, float textSize, std::function<void()> onClick);
