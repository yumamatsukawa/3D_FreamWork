#pragma once

// 1フレームの時間・起動してからの時間・時間の倍率(スロー/ポーズ)をまとめたもの。
// 1フレームの時間には上限(0.1秒)があり、ウィンドウのドラッグ中などでも大きな値にならない
// ※Engineフォルダをインクルードパス(AdditionalIncludeDirectories)に追加しないこと。標準の<time.h>とファイル名がぶつかる
namespace Time {
    float GetDeltaTime();             // 1フレームの時間(秒)。倍率の影響を受ける(ポーズ中は0)
    float GetUnscaledDeltaTime();     // 1フレームの本当の時間(秒)。倍率の影響を受けない(ポーズ中も動かしたいUIなど用)
    float GetTime();                  // 起動してからの時間(秒)。倍率の影響を受ける
    void  SetTimeScale(float scale);  // 時間の倍率(1=通常、0.3=スロー、0=ポーズ)。負の値は0にする
    float GetTimeScale();             // 今の倍率
    bool  IsPaused();                 // ポーズ中か(倍率が0の時true)

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Init();     // 時間の計測を始める
    void Update();   // 毎フレーム1回、このフレームの時間を計算する
}
