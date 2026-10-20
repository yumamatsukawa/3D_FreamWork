#include "DemoCategoryMenuController.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/UI/ButtonComponent.h"
#include "../../../../Engine/UI/TextRenderer.h"

void DemoCategoryMenuController::AddRow(const DemoInfo* demo, ButtonComponent* button, TextRenderer* text) {
    rows.push_back({ demo, button, text, -1 });
}

void DemoCategoryMenuController::Update(float dt) {
    const DemoMode mode = GetDemoMode();

    for (Row& row : rows) {
        // 変わった時だけ反映する(今の側と、その側の版があるか)
        const bool openable = CanOpenDemo(*row.demo, mode);
        const int state = (openable ? 1 : 0) + (mode == DemoMode::Mode3D ? 2 : 0);
        if (state == row.state) continue;
        row.state = state;

        // ボタン: その側の版が無い機能は、暗くして押せなくする
        row.button->SetInteractable(openable);

        // 説明: 印([2D/3D]など)+一言説明。押せない時は理由も足す
        std::wstring description = std::wstring(GetDemoKindTag(*row.demo)) + L" " + row.demo->description;
        if (!openable) description += (mode == DemoMode::Mode2D) ? L"  (2D版はありません)" : L"  (3D版はありません)";
        row.text->SetText(description);
    }
}
