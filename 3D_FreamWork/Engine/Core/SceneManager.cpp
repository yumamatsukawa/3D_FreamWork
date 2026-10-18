#include "SceneManager.h"
#include "Resources.h"
#include "../Render/ScreenEffect.h"
#include "../Render/CameraShake.h"
#include "Time.h"
#include <windows.h>

void SceneManager::RequestChange(std::function<Scene* ()> factory, std::unique_ptr<SceneTransition> newTransition) {
    // 切り替えの途中(頼まれてからシーンが切り替わるまで・演出が終わるまで)は無視する(二重押しなどで2回切り替わらないように)
    if (nextScene || transition) {
        OutputDebugStringA("★ SceneManager::ChangeScene: シーンを切り替えている途中なので無視しました\n");
        return;
    }

    nextScene = std::move(factory);

    // 演出付き: 覆い始めて、演出が終わるまでゲームの時間を止める
    if (newTransition) {
        transition = std::move(newTransition);
        transition->Begin();
        Time::PauseForSceneTransition();
    }
}

void SceneManager::SwitchScene() {
    if (currentScene) currentScene->Uninit();

    // 前のシーンの演出を片付ける(Flash・Vignette・揺れを消し、Fadeの完了の関数を捨てる。Fadeの濃さは残す)。
    // 古いシーンのUninitの後・新しいシーンのInitの前に呼ぶので、新しいシーンのInitで始めた明転や揺れは消えない
    ScreenEffect::OnSceneChanged();
    CameraShake::Stop();

    // nextScene は新しいシーンのInitが終わってから空にする(古いシーンのUninit・新しいシーンのInitの中の ChangeScene は無視される)
    currentScene.reset(nextScene());
    currentScene->Init();
    nextScene = nullptr;

    // 古いシーンだけが使っていた画像を解放する。
    // 新しいシーンのInitの後に呼ぶので、両方のシーンで使う画像は読み直さずにそのまま使い回せる
    Resources::UnloadUnused();
}

void SceneManager::Update(float dt) {
    if (transition) {
        // 演出付き: 演出の時間はポーズ中も進める(ゲームの時間は止めているため)
        transition->Update(Time::GetUnscaledDeltaTime());

        // 覆い終わった: 画面が隠れている間にシーンを切り替えて、開き始める
        if (transition->IsCovered()) {
            SwitchScene();
            transition->BeginOpen();
        }

        // 開き終わった: 演出を片付けて、ゲームの時間を元の倍率に戻す
        if (transition->IsFinished()) {
            transition.reset();
            Time::ResumeAfterSceneTransition();
        }
    } else if (nextScene) {
        // 演出なし: すぐに切り替える
        SwitchScene();
    }

    if (currentScene) currentScene->Update(dt);
}

void SceneManager::Draw() {
    if (currentScene) currentScene->Draw();
}

void SceneManager::DrawTransition() {
    if (transition) transition->Draw();
}

void SceneManager::Uninit() {
    // 演出の途中で終了した時: 演出を片付けて(ブラシの解放)、時間の倍率も戻しておく
    if (transition) {
        transition.reset();
        Time::ResumeAfterSceneTransition();
    }
    nextScene = nullptr;

    if (currentScene) {
        currentScene->Uninit();
        currentScene.reset();
    }
}
