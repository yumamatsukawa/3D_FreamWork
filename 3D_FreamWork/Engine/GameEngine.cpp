// GameEngine.cpp
#include "GameEngine.h"
#include "Input.h"
#include "Image.h"
#include "SceneManager.h"
#include "Collider.h"
#include "Audio.h"
#include "Text.h"

bool GameEngine::Init(HWND hwnd, int w, int h)
{
    if (!Image::Init(hwnd, w, h)) return false;
    if (!Audio::Init())           return false;
    if (!Text::Init())            return false;
    InitCollider();
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
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

        float dt = CalcDeltaTime();

        Input::Update();
        SceneManager::Get().Update(dt);
        UpdateCollider();

        Image::BeginFrame();
        Text::BeginDraw();
        SceneManager::Get().Draw();
        Text::EndDraw();

        DrawColliders();
        Image::EndFrame();
    }
}

void GameEngine::Uninit()
{
    SceneManager::Get().Uninit();
    UninitCollider();
    Text::Uninit();
    Audio::Uninit();
    Image::Uninit();
}

float GameEngine::CalcDeltaTime()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    float dt = float(now.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
    lastTime = now;
    return dt;
}