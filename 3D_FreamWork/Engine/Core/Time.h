#pragma once

// 1フレームの時間・起動してからの時間・時間の倍率(スロー/ポーズ)をまとめたもの。
// 1フレームの時間には上限(0.1秒)があり、ウィンドウのドラッグ中などでも大きな値にならない
// シーン切り替えの演出(SceneTransition)の間は、時間が止まる(倍率0)。演出が終わると、切り替える前の倍率に戻る
// ※Engineフォルダをインクルードパス(AdditionalIncludeDirectories)に追加しないこと。標準の<time.h>とファイル名がぶつかる
namespace Time {
    float GetDeltaTime();             // 1フレームの時間(秒)。倍率の影響を受ける(ポーズ中は0)
    float GetUnscaledDeltaTime();     // 1フレームの本当の時間(秒)。倍率の影響を受けない(ポーズ中も動かしたいUIなど用)
    float GetTime();                  // 起動してからの時間(秒)。倍率の影響を受ける
    // 時間の倍率(1=通常、0.3=スロー、0=ポーズ)。負の値は0にする。
    // シーン切り替えの演出中に呼んだ時は、すぐには変わらず(止まったまま)、演出が終わった時にその倍率になる(最後に呼んだ値)
    void  SetTimeScale(float scale);
    float GetTimeScale();             // 今の倍率(シーン切り替えの演出中は0)
    bool  IsPaused();                 // ポーズ中か(倍率が0の時true。シーン切り替えの演出中もtrue)

    // ─── Engine内部用(GameEngine・SceneManagerだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Init();     // 時間の計測を始める
    void Update();   // 毎フレーム1回、このフレームの時間を計算する
    void PauseForSceneTransition();     // シーン切り替えの演出が始まった: 今の倍率を覚えて、倍率を0にする
    void ResumeAfterSceneTransition();  // 演出が終わった: 覚えておいた倍率(演出中にSetTimeScaleされたらその値)に戻す
}
