#include "DemoButton.h"
#include "../../Components/Common/DemoScreenFixController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/UI/ButtonComponent.h"

GameObject* CreateDemoButton(Scene& scene, const std::wstring& label, DirectX::XMFLOAT2 position,
    DirectX::XMFLOAT2 size, float textSize, std::function<void()> onClick) {
    // 位置と大きさ(ButtonComponentを付ける前に決める。文字の大きさの基準になるため)
    GameObject* button = scene.CreateObject("DemoButton");
    button->transform.position = { position.x, position.y, 0.f };
    button->transform.scale = { size.x, size.y, 1.f };

    // 2Dカメラが動くデモ(Camera・Coord)でも、ボタンが画面の同じ場所に見えるようにする(描く前に位置を直すので、一番先に付ける)
    button->AddComponent<DemoScreenFixController>();

    // 見た目
    button->AddComponent<SpriteRenderer>(L"Assets/button.png");

    // クリック判定・文字の表示
    button->AddComponent<ButtonComponent>(label, textSize, std::move(onClick));
    return button;
}
