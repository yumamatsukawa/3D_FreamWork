// GameEngine.cpp
#include "GameEngine.h"
#include "Input.h"
#include "Image.h"
#include "SceneManager.h"
#include "Audio.h"
#include "Text.h"
#include "Physics.h"
#include "Time.h"
#include "Resources.h"
#include "CameraShake.h"
#include "ScreenEffect.h"
#include "SpriteTransition.h"
#include "SaveData.h"

bool GameEngine::Init(HWND hwnd, int w, int h)
{
    SaveData::Init();   // セーブデータを読み込む(ほかの初期化に失敗しても読み込んでおく。読めなくてもゲームは始める)
    if (!Image::Init(hwnd, w, h)) return false;
    if (!Audio::Init())           return false;
    if (!Text::Init())            return false;
    if (!Physics::Init())         return false;
    Time::Init();
    isRunning = true;
    return true;
}

void GameEngine::Run()
{
    MSG msg = {};
    while (isRunning) {
        // Windowsメッセージ処理
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) isRunning = false;
        }

        Time::Update();

        Input::Update();
        CameraShake::Update(Time::GetUnscaledDeltaTime());   // 画面の揺れはポーズ中も進める
        ScreenEffect::Update(Time::GetUnscaledDeltaTime());  // 画面エフェクトもポーズ中も進める(Fadeの完了の関数はここで呼ばれる。中でChangeSceneすると、すぐ下のSceneManager::Updateで切り替わる)

        Physics::Update(Time::GetDeltaTime());
        SceneManager::Get().Update(Time::GetDeltaTime());

        // 画面の揺れは描画の間だけカメラをずらす(Image::BeginFrameで3Dカメラを2Dカメラに合わせた後にずらし、
        // 描き終わったら元に戻す。Updateの中のカメラの値には揺れが入らない)
        Image::BeginFrame();
        CameraShake::BeginDraw();
        Text::BeginDraw();
        SceneManager::Get().Draw();
        ScreenEffect::Draw();   // 画面エフェクトは文字も含めて一番上に出す(Direct2Dは後に描いた物が上になるので、シーンの描画の後・EndDrawの前)
        SceneManager::Get().DrawTransition();   // シーン切り替えの演出は、画面エフェクトよりもさらに上に出す
        Text::EndDraw();
        CameraShake::EndDraw();

        Image::EndFrame();
    }
}

void GameEngine::Uninit()
{
    SceneManager::Get().Uninit();   // 切り替えの演出の途中なら、演出(ブラシ)もここで片付く
    SaveData::Uninit();             // 書き換えていれば保存する(シーンのUninitで書いた値も残るように、SceneManagerの後)
    SpriteTransition::UnloadAll();  // Text::Uninitより先に(切り替えの演出の画像はTextの描画先から作っているため)
    Resources::UnloadAll();  // Image::Uninitより先に(画像の解放にImageを使うため)
    Physics::Uninit();
    ScreenEffect::Uninit();   // Text::Uninitより先に(ブラシはTextの描画先から作っているため)
    Image::ReleaseDrawRectBrush();   // 同じ理由でText::Uninitより先に
    Text::Uninit();
    Audio::Uninit();
    Image::Uninit();
}