#include "Time.h"
#include <windows.h>

namespace {
    // 1フレームの時間の上限(秒)。これより長い時間を一度に進めると、すり抜け・ワープが起きるため
    constexpr float maxDeltaTime = 0.1f;

    LARGE_INTEGER frequency{};        // 1秒あたりのカウント数
    LARGE_INTEGER lastTime{};         // 前のフレームのカウント
    float unscaledDeltaTime = 0.f;    // 1フレームの本当の時間(上限付き)
    float deltaTime = 0.f;            // 1フレームの時間(倍率込み)
    double totalTime = 0.0;           // 起動してからの時間(倍率込み。長時間足し続けても誤差が出にくいようdoubleで持つ)
    float timeScale = 1.f;            // 時間の倍率
    bool pausedForTransition = false; // シーン切り替えの演出で止めている間か
    float resumeTimeScale = 1.f;      // 演出が終わった時に戻す倍率
}

namespace Time {
    float GetDeltaTime() { return deltaTime; }
    float GetUnscaledDeltaTime() { return unscaledDeltaTime; }
    float GetTime() { return (float)totalTime; }

    void SetTimeScale(float scale) {
        // 時間を巻き戻すことはできないので、負の値は0(ポーズ)にする(NaNもここで0になる)
        if (!(scale >= 0.f)) scale = 0.f;

        // シーン切り替えの演出中は止めたままにして、終わった時に戻す倍率として覚えるだけにする
        // (新しいシーンのInitで SetTimeScale(1) した時なども、演出が終わってから効く)
        if (pausedForTransition) {
            resumeTimeScale = scale;
            return;
        }
        timeScale = scale;
    }

    float GetTimeScale() { return timeScale; }
    bool IsPaused() { return timeScale == 0.f; }

    void PauseForSceneTransition() {
        if (pausedForTransition) return;
        pausedForTransition = true;
        resumeTimeScale = timeScale;
        timeScale = 0.f;
    }

    void ResumeAfterSceneTransition() {
        if (!pausedForTransition) return;
        pausedForTransition = false;
        timeScale = resumeTimeScale;
    }

    void Init() {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&lastTime);
        unscaledDeltaTime = 0.f;
        deltaTime = 0.f;
        totalTime = 0.0;
    }

    void Update() {
        // 前のフレームからの時間を計る
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = float(now.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
        lastTime = now;

        // 上限: ウィンドウのドラッグ中などで止まっていた分を、1フレームでまとめて進めない
        // (倍率をかける前の本当の時間にかけるので、GetUnscaledDeltaTimeも上限付きになる)
        if (dt > maxDeltaTime) dt = maxDeltaTime;

        unscaledDeltaTime = dt;
        deltaTime = dt * timeScale;
        totalTime += deltaTime;
    }
}
