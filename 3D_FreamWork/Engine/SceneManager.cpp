#include "SceneManager.h"

void SceneManager::Update(float dt) {
    if (isChanging && nextScene) {
        if (currentScene) currentScene->Uninit();
        currentScene.reset(nextScene());  // ★ factories は不要
        currentScene->Init();
        nextScene = nullptr;
        isChanging = false;
    }
    if (currentScene) currentScene->Update(dt);
}

void SceneManager::Draw() {
    if (currentScene) currentScene->Draw();
}

void SceneManager::Uninit() {
    if (currentScene) {
        currentScene->Uninit();
        currentScene.reset();
    }
}