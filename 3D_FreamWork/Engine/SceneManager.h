#pragma once
#include <memory>
#include <functional>
#include "Scene.h"

class SceneManager {
public:
    static SceneManager& Get() {
        static SceneManager instance;
        return instance;
    }

    template<typename T>
    void ChangeScene() {
        // ★ std::function に明示的に代入
        nextScene = std::function<Scene * ()>([]() -> Scene* { return new T(); });
        isChanging = true;
    }

    void Update(float dt);
    void Draw();
    void Uninit();

private:
    SceneManager() = default;

    std::unique_ptr<Scene>  currentScene;
    std::function<Scene* ()> nextScene = nullptr;  // ★ nullptr で初期化
    bool                    isChanging = false;
};