// GameEngine.cpp
#include "GameEngine.h"
#include "Input.h"
#include "Image.h"
#include "SceneManager.h"
#include "Audio.h"
#include "Text.h"
#include "Physics.h"

bool GameEngine::Init(HWND hwnd, int w, int h)
{
    if (!Image::Init(hwnd, w, h)) return false;
    if (!Audio::Init())           return false;
    if (!Text::Init())            return false;
    if (!Physics::Init())         return false;
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
        // ★ PhysicsをSceneManagerより先に更新する。こうすることで、RigidbodyComponentの
        //   Update()が「このフレームで今シミュレーションされたばかりの姿勢」を
        //   Transformへ同期できる(順番が逆だと1フレーム古い姿勢を読むことになる)
        Physics::Update(dt);
        SceneManager::Get().Update(dt);

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
    Physics::Uninit();
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