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

bool GameEngine::Init(HWND hwnd, int w, int h)
{
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

        Physics::Update(Time::GetDeltaTime());
        SceneManager::Get().Update(Time::GetDeltaTime());

        // 画面の揺れは描画の間だけカメラをずらす(Image::BeginFrameで3Dカメラを2Dカメラに合わせた後にずらし、
        // 描き終わったら元に戻す。Updateの中のカメラの値には揺れが入らない)
        Image::BeginFrame();
        CameraShake::BeginDraw();
        Text::BeginDraw();
        SceneManager::Get().Draw();
        Text::EndDraw();
        CameraShake::EndDraw();

        Image::EndFrame();
    }
}

void GameEngine::Uninit()
{
    SceneManager::Get().Uninit();
    Resources::UnloadAll();   // Image::Uninitより先に(画像の解放にImageを使うため)
    Physics::Uninit();
    Text::Uninit();
    Audio::Uninit();
    Image::Uninit();
}