#include "Button.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render/SpriteRenderer.h"
#include "../../../Engine/UI/ButtonComponent.h"

GameObject* CreateButton(Scene& scene, const std::wstring& label, Transform tf,
    std::function<void()> onClick) {
    // 位置と大きさ
    GameObject* button = scene.CreateObject("Button");
    button->transform.position = { tf.position.x, tf.position.y, 0.f };
    button->transform.scale = { tf.scale.x, tf.scale.y, 1.f };

    // 見た目
    button->AddComponent<SpriteRenderer>(L"Assets/button.png");

    // クリック判定・文字の表示(文字の大きさ48)
    button->AddComponent<ButtonComponent>(label, 48.f, std::move(onClick));
    return button;
}
