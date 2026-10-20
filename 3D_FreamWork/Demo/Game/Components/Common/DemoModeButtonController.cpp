#include "DemoModeButtonController.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/UI/ButtonComponent.h"

void DemoModeButtonController::Init() {
    // 同じオブジェクトに付いているComponentを探す(作る側で、このComponentより先に付けておく)
    button = GetOwner()->GetComponent<ButtonComponent>();
}

void DemoModeButtonController::Update(float dt) {
    if (!button) return;
    const bool mode3D = (GetDemoMode() == DemoMode::Mode3D);

    // 文字と、押せるかを決める
    std::wstring label;
    bool interactable = true;
    if (screen != DemoScreen::Demo) {
        // メニュー: 今選んでいる側を出す(押すと入れ替わる)
        label = mode3D ? L"Mode: 3D" : L"Mode: 2D";
    } else if (CanSwitchDemoMode()) {
        // 2D版と3D版がある機能: 逆の側へ切り替える
        label = mode3D ? L"To 2D" : L"To 3D";
    } else {
        // 切り替えられない機能: 暗くして、理由を出す(2D・3Dの区別が無い / 片方だけ)
        interactable = false;
        const DemoInfo* demo = GetCurrentDemo();
        if (demo && demo->open2D == demo->open3D) label = L"Common";
        else label = (demo && demo->open3D) ? L"3D only" : L"2D only";
    }

    // 変わった時だけ反映する
    if (label != lastLabel) {
        button->SetLabel(label);
        lastLabel = label;
    }
    if ((int)interactable != lastInteractable) {
        button->SetInteractable(interactable);
        lastInteractable = (int)interactable;
    }
}
