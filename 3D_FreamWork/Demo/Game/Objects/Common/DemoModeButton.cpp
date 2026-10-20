#include "DemoModeButton.h"
#include "DemoButton.h"
#include "../../Components/Common/DemoModeButtonController.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/UI/Anchor.h"

GameObject* CreateDemoModeButton(Scene& scene, DemoScreen screen) {
    // 押された時: メニューは側を入れ替えるだけ、デモ本体は逆の側の版へ切り替える(Tabキーと同じ)
    std::function<void()> onClick = (screen == DemoScreen::Demo) ? std::function<void()>(SwitchDemoMode) : std::function<void()>(ToggleDemoMode);

    // ボタン(画面の右下から左へ130、上へ50の所に、中心が来る)
    GameObject* button = CreateDemoButton(scene, L"Mode", GetAnchoredPosition(Anchor::BottomRight, { -130.f, 50.f }),
        { 220.f, 56.f }, 26.f, onClick);

    // 文字と押せるかを、状態に合わせて毎フレーム決める役(ボタンのComponentより後に付ける)
    button->AddComponent<DemoModeButtonController>(screen);
    return button;
}
