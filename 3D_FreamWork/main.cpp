#include <windows.h>
#include "Engine/GameEngine.h"
#include "Engine/Graphics.h"
#include "Engine/Input.h"
#include "Engine/SceneManager.h"
#include "Game/Scenes/TitleScene.h"

#define WINDOW_CLASS_NAME (L"GameWindow")
#define WINDOW_NAME (L"ブロック崩し")
#define WINDOW_W (1280)
#define WINDOW_H (720)

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    // ウィンドウ作成
    WNDCLASSEX wc = {
    sizeof(wc),       // 構造体のサイズ（必須・固定値）
    CS_CLASSDC,       // ウィンドウクラスのスタイル
    WndProc,          // メッセージ処理関数のポインタ
    0,                // クラス領域の追加バイト数（通常0）
    0,                // ウィンドウ領域の追加バイト数（通常0）
    hInst,            // このアプリのインスタンスハンドル
    nullptr,          // アイコン（nullptrでデフォルト）
    nullptr,          // カーソル（nullptrでデフォルト）
    nullptr,          // 背景ブラシ（DX11は自分で描くのでnullptr）
    nullptr,          // メニュー名（ゲームは不要）
    L"DX11Game",      // クラス名（CreateWindowと一致させる）
    nullptr           // 小アイコン（nullptrでデフォルト）
    };

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(
        L"DX11Game",        // WNDCLASSEXで登録したクラス名
        L"My DX11 Game",    // タイトルバーに表示される文字
        WS_OVERLAPPEDWINDOW,// ウィンドウのスタイル
        100,                // 画面左からの位置（ピクセル）
        50,                // 画面上からの位置（ピクセル）
        1280,               // ウィンドウの幅
        720,                // ウィンドウの高さ
        nullptr,            // 親ウィンドウ（トップレベルなのでnullptr）
        nullptr,            // メニュー（ゲームは不要）
        hInst,              // アプリのインスタンスハンドル
        nullptr             // 追加データ（今は不要）
    );

    ShowWindow(hwnd, SW_SHOW);

    Graphics::Init;
    Input::SetHwnd(hwnd);
     
    auto& engine = GameEngine::Get();
    engine.Init(hwnd, WINDOW_W, WINDOW_H);

    // 最初のシーンの設定
    SceneManager::Get().ChangeScene<TitleScene>();

    engine.Run();
    engine.Uninit();
    Graphics::Uninit;

    return 0;
}