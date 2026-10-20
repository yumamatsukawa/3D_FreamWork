#include "DemoGuide.h"
#include "DemoButton.h"
#include "DemoModeButton.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/UI/Anchor.h"

GameObject* CreateDemoGuide(Scene& scene, const DemoGuideInfo& info, DemoScreen screen) {
    // 戻る先: カテゴリ一覧は終了、機能一覧はカテゴリ一覧へ、デモ本体は機能一覧へ
    void (*onBack)() = OpenCurrentCategory;
    if (screen == DemoScreen::Top) onBack = QuitDemo;
    else if (screen == DemoScreen::List) onBack = OpenDemoMenu;

    // 説明の表示と、Escキー・Tabキーの処理
    GameObject* guide = scene.CreateObject("DemoGuide");
    guide->AddComponent<DemoGuideController>(info, onBack, screen);

    // 戻るボタン(画面の左下から右へ110、上へ50の所に、ボタンの中心が来る)
    CreateDemoButton(scene, (screen == DemoScreen::Top) ? L"Quit" : L"Back", GetAnchoredPosition(Anchor::BottomLeft, { 110.f, 50.f }),
        { 160.f, 56.f }, 28.f, onBack);

    // 2D/3D切り替えボタン(右下)
    CreateDemoModeButton(scene, screen);
    return guide;
}
