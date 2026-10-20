#pragma once
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Core/Timer.h"

class GameObject;

// Timeデモの操作役。SPACEでスロー、Pでポーズ、1〜3でTimerを数え始め、時間の値・Timerの進み・回る2つの物を見せる。
// 時間の倍率は全体の設定なので、Uninitで必ず1に戻す
class TimeDemoController : public Component {
private:
    // Timer1つ分(数えているか・時間が来た回数と一緒に持つ)
    struct TimerRow {
        const wchar_t* label;    // 画面に出す説明
        Timer timer;             // 数えるタイマー
        bool running;            // 数えている最中か(繰り返さないタイマーは、時間が来たら止まる)
        int firedCount;          // 時間が来た回数
    };

    static constexpr float slowScale = 0.3f;     // スローの時の倍率
    static constexpr float spinSpeed = 150.f;    // 回る物の速さ(度 / 秒)

    GameObject* scaledObject;        // 倍率の影響を受ける dt で回る物
    GameObject* unscaledObject;      // 倍率の影響を受けない UnscaledDeltaTime で回る物
    TimerRow rows[3];                // 1・2・3キーのタイマー
    bool slow = false;               // スロー中か
    bool paused = false;             // このデモがポーズしているか(シーン切り替えの演出によるポーズとは別)

    void ApplyTimeScale() const;     // slow / paused に合わせて Time::SetTimeScale を呼ぶ

public:
    TimeDemoController(GameObject* scaled, GameObject* unscaled);

    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
