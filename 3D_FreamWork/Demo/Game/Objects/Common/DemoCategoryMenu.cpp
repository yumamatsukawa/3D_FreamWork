#include "DemoCategoryMenu.h"
#include "DemoMenu.h"
#include "../../Components/Common/DemoCategoryMenuController.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/UI/ButtonComponent.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/TextRenderer.h"

GameObject* CreateDemoCategoryMenu(Scene& scene) {
    // 行の見た目(暗さ・説明)を側に合わせて更新する役
    GameObject* menu = scene.CreateObject("DemoCategoryMenu");
    auto* controller = menu->AddComponent<DemoCategoryMenuController>();

    // 今のカテゴリの機能ごとに、ボタンと説明を1行ずつ
    const std::vector<const DemoInfo*> demos = GetDemoListOf(GetCurrentCategory());
    for (size_t i = 0; i < demos.size(); i++) {
        const DemoInfo* demo = demos[i];
        GameObject* row = CreateDemoMenuRow(scene, demo->name, L"", (int)i, (int)demos.size(), [demo] { OpenDemo(*demo); });
        controller->AddRow(demo, row->GetComponent<ButtonComponent>(), row->GetComponent<TextRenderer>());
    }

    // 機能がまだ無いカテゴリ
    if (demos.empty()) {
        TextStyle style;
        style.size = 32.f;
        style.fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
        style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        GameObject* message = scene.CreateObject("EmptyMessage");
        auto* text = message->AddComponent<TextRenderer>();
        text->SetText(L"このカテゴリのデモは、これから追加されます");
        text->SetStyle(style);
    }
    return menu;
}
