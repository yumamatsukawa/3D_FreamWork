#include "SceneManager.h"
#include "Resources.h"

void SceneManager::Update(float dt) {
    if (isChanging && nextScene) {
        if (currentScene) currentScene->Uninit();
        currentScene.reset(nextScene());
        currentScene->Init();
        nextScene = nullptr;
        isChanging = false;

        // 古いシーンだけが使っていた画像を解放する。
        // 新しいシーンのInitの後に呼ぶので、両方のシーンで使う画像は読み直さずにそのまま使い回せる
        Resources::UnloadUnused();
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
