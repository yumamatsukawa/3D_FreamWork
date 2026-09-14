// GameEngine.h
#pragma once
#include <windows.h>
#include "Scene.h"

class GameEngine {
private:
    GameEngine() = default;
    Scene    scene;

    LARGE_INTEGER frequency{}, lastTime{};
    bool isRunning = false;

    float CalcDeltaTime();
public:
    static GameEngine& Get() {          // シングルトン
        static GameEngine instance;
        return instance;
    }

    bool Init(HWND hwnd, int w, int h);
    void Run();   // メインループ
    void Uninit();

    Scene& GetScene() { return scene; }
};