#include "DemoMenu.h"
#include "DemoButton.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/UI/ButtonComponent.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/TextRenderer.h"
#include <algorithm>

namespace {
    constexpr float firstRowY = 170.f;                      // 1行目のボタンの中心のY
    constexpr float maxRowGap = 84.f;                       // 行の間隔の最大(行が少ない時)
    constexpr float listHeight = 440.f;                     // 一覧に使える高さ(行が増えたら、この中に収まるよう間隔を縮める)
    constexpr float buttonX = -380.f;                       // ボタンの中心のX
    constexpr DirectX::XMFLOAT2 buttonSize = { 300.f, 64.f };   // ボタンの大きさ
    constexpr float descriptionOffsetX = 190.f;             // 説明の左端は、ボタンの中心からこれだけ右
}

GameObject* CreateDemoMenuRow(Scene& scene, const std::wstring& name, const std::wstring& description,
    int row, int rowCount, std::function<void()> onClick) {
    const std::wstring fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");

    // 行の間隔(行の数で決まる)とボタンの高さ
    const float rowGap = (std::min)(maxRowGap, listHeight / (float)(std::max)(1, rowCount));
    const float buttonHeight = (std::min)(buttonSize.y, rowGap - 10.f);

    // ボタン
    GameObject* button = CreateDemoButton(scene, name, { buttonX, firstRowY - rowGap * (float)row },
        { buttonSize.x, buttonHeight }, 30.f, std::move(onClick));

    // 一言説明(ボタンの右に、左寄せで)
    TextStyle style;
    style.size = 21.f;
    style.fontName = fontName;
    style.align = TextAlign::Left;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    auto* text = button->AddComponent<TextRenderer>();
    text->SetText(description);
    text->SetStyle(style);
    text->SetOffset({ descriptionOffsetX, 0.f, 0.f });
    return button;
}

GameObject* CreateDemoMenu(Scene& scene) {
    const std::vector<DemoCategoryInfo>& categories = GetCategoryList();

    GameObject* last = nullptr;
    for (size_t i = 0; i < categories.size(); i++) {
        const DemoCategory category = categories[i].category;
        const size_t count = GetDemoListOf(category).size();

        // 説明の最後に、デモの数(無ければ準備中)を付ける
        std::wstring description = categories[i].description;
        description += count > 0 ? L"  [" + std::to_wstring(count) + L"]" : std::wstring(L"  (準備中)");

        // ボタン(押すとそのカテゴリの機能一覧へ。デモが1つも無いカテゴリは押せない)
        last = CreateDemoMenuRow(scene, categories[i].name, description, (int)i, (int)categories.size(),
            [category] { OpenCategory(category); });
        last->GetComponent<ButtonComponent>()->SetInteractable(count > 0);
    }
    return last;
}
