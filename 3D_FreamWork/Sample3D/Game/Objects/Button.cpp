#include "Button.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../Components/ButtonController.h"

GameObject* CreateButton(Scene& scene, const std::wstring& label, DirectX::XMFLOAT2 position,
    std::function<void()> onClick) {
    // 位置と大きさ
    GameObject* button = scene.CreateObject("Button");
    button->transform.position = { position.x, position.y, 0.f };
    button->transform.scale = { 320.f, 96.f, 1.f };

    // 見た目
    button->AddComponent<SpriteRenderer>(L"Assets/button.png");

    // クリック判定・文字の表示(文字の大きさ48)
    button->AddComponent<ButtonController>(label, 48.f, std::move(onClick));
    return button;
}
