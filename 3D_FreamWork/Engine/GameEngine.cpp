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

        Physics::Update(Time::GetDeltaTime());
        SceneManager::Get().Update(Time::GetDeltaTime());

        Image::BeginFrame();
        Text::BeginDraw();
        SceneManager::Get().Draw();
        Text::EndDraw();

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