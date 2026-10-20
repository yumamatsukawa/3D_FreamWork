#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

// デモの説明(画面の左上に出す4行)
struct DemoGuideInfo {
    std::wstring name;          // デモの名前
    std::wstring description;   // 何のデモか
    std::wstring controls;      // 操作方法
    std::wstring api;           // 使っているEngineのAPI
};

// どの画面か
enum class DemoScreen {
    Top,    // カテゴリ一覧(一番上。Escでアプリを終了。Tabで側を選ぶ)
    List,   // 機能一覧(Escでカテゴリ一覧へ。Tabで側を選ぶ)
    Demo,   // デモ本体(Escで機能一覧へ。Tabで2D版と3D版を切り替える)
};

// デモ画面の左上の説明の表示と、Esc/Tabキーの処理を担当する(画面に出入りする時の、全体の設定の初期化も)
class DemoGuideController : public Component {
private:
    DemoGuideInfo info;            // 表示する説明
    void (*onBack)() = nullptr;    // Escキーが押された時の処理(前の画面へ戻る)
    DemoScreen screen;             // どの画面か
    std::wstring fontName;         // 文字のフォント(InitでText::LoadFontした名前)

    static void ResetSettings();   // デモが変えるかもしれない全体の設定を、すべて既定に戻す(ここ1か所に集める)

public:
    DemoGuideController(const DemoGuideInfo& guideInfo, void (*backAction)(), DemoScreen guideScreen)
        : info(guideInfo), onBack(backAction), screen(guideScreen) {}

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
