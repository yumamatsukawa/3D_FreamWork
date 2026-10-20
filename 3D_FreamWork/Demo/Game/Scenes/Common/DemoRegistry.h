#pragma once
#include <string>
#include <vector>

// 今見ている側(2D版か3D版か)
enum class DemoMode { Mode2D, Mode3D };

// デモの分け方(Engineのフォルダに近い名前。DemoCategoryScene が、この単位で機能の一覧を出す)
enum class DemoCategory { Core, Render, UI, Render3D, Physics, Behavior, Audio, Data };

// デモ(機能)1つ分の情報(機能一覧のボタンと説明になる)
// open2D / open3D が nullptr かどうかが、その機能の「2Dあり/3Dあり」の印になる。
// 2Dと3Dの区別が無い機能は、両方に同じ関数を入れる(どちらの側でも同じデモが開く)
struct DemoInfo {
    DemoCategory category;        // どのカテゴリか
    const wchar_t* name;          // ボタンに出す名前(短い英語)
    const wchar_t* description;   // 一言説明(ボタンの右に出す)
    void (*open2D)();             // 2D版を開く関数(無ければ nullptr)
    void (*open3D)();             // 3D版を開く関数(無ければ nullptr)
};

// カテゴリ1つ分の情報(カテゴリ一覧のボタンと説明になる)
struct DemoCategoryInfo {
    DemoCategory category;
    const wchar_t* name;          // ボタンに出す名前(短い英語)
    const wchar_t* description;   // 一言説明(ボタンの右に出す)
};

// ─── 一覧(新しいデモの追加は DemoRegistry.cpp の GetDemoList に1行足すだけ) ───
const std::vector<DemoInfo>& GetDemoList();
std::vector<const DemoInfo*> GetDemoListOf(DemoCategory category);
const std::vector<DemoCategoryInfo>& GetCategoryList();
const DemoCategoryInfo& GetCategoryInfo(DemoCategory category);

// ─── 今の状態(アプリ全体で1つ。シーンを切り替えても残る) ───
DemoMode GetDemoMode();                           // 今の側(起動した時は3D)
void SetDemoMode(DemoMode mode);
void ToggleDemoMode();                            // 側を入れ替えるだけ(メニュー用。シーンは切り替えない)
DemoCategory GetCurrentCategory();                // 今開いている(開いた)カテゴリ
const DemoInfo* GetCurrentDemo();                 // 最後に開いたデモ(デモ画面の切り替えと印が読む。まだ開いていない時は nullptr)
bool CanOpenDemo(const DemoInfo& demo, DemoMode mode);   // その側の版があるか
bool CanSwitchDemoMode();                         // 今のデモに、逆の側の別の版があるか
const wchar_t* GetDemoKindTag(const DemoInfo& demo);     // 機能一覧の印: [2D/3D] [2D] [3D] [ALL]
std::wstring GetCurrentModeTag();                 // デモ画面の名前に付ける印: [2D] [3D] [ALL]

// ─── 画面の出入り口(暗転して切り替える) ───
void OpenDemoMenu();                              // カテゴリ一覧へ
void OpenCategory(DemoCategory category);         // そのカテゴリの機能一覧へ
void OpenCurrentCategory();                       // 今のカテゴリの機能一覧へ(デモの「戻る」)
void OpenDemo(const DemoInfo& demo);              // 今の側の版でデモを開く(その側の版が無ければ何もしない)
void SwitchDemoMode();                            // 今のデモの、逆の側の版へ切り替える(出来ない時は何もしない)
void QuitDemo();                                  // アプリを終了する
