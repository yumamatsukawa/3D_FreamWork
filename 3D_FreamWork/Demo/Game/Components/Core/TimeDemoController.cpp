#include "TimeDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/Image.h"
#include <cstdio>

namespace {
    constexpr float infoX = -610.f;       // 時間の値の文字の左端
    constexpr float infoY = 190.f;        // 時間の値の1行目(説明の板の下)
    constexpr float infoGap = 30.f;       // 行の間隔
    constexpr float timerX = 60.f;        // タイマーの表の左端
    constexpr float timerY = 180.f;       // タイマーの1行目
    constexpr float timerGap = 72.f;      // タイマーの行の間隔
    constexpr float barWidth = 400.f;     // 進みの帯の幅
    constexpr float barHeight = 14.f;     // 進みの帯の高さ
}

TimeDemoController::TimeDemoController(GameObject* scaled, GameObject* unscaled)
    : scaledObject(scaled), unscaledObject(unscaled),
    rows{ { L"1: 2秒で1回だけ時間が来る", Timer(2.f), false, 0 },
          { L"2: 1秒ごとに時間が来る(もう一度押すと止める)", Timer(1.f, true), false, 0 },
          { L"3: 5秒で1回だけ時間が来る", Timer(5.f), false, 0 } } {}

void TimeDemoController::ApplyTimeScale() const {
    // ポーズは倍率0、スローは0.3、通常は1
    Time::SetTimeScale(paused ? 0.f : (slow ? slowScale : 1.f));
}

void TimeDemoController::Update(float dt) {
    // 回る2つの物: dt は倍率の影響を受ける(ポーズ中は0)、UnscaledDeltaTime は受けない(ポーズ中も進む)
    scaledObject->transform.rotate.z += spinSpeed * dt;
    unscaledObject->transform.rotate.z += spinSpeed * Time::GetUnscaledDeltaTime();

    // シーン切り替えの演出中(このデモがポーズしていないのに止まっている間)は、操作を受け付けない
    if (Time::IsPaused() && !paused) return;

    // SPACE: スロー / P: ポーズ
    if (Input::GetKeyDown(KEY_SPACE)) {
        slow = !slow;
        ApplyTimeScale();
    }
    if (Input::GetKeyDown(KEY_P)) {
        paused = !paused;
        ApplyTimeScale();
    }

    // 1〜3: タイマーを最初から数え始める(2は繰り返すタイマーなので、押す度に数える/止めるを切り替える)
    const int keys[3] = { KEY_1, KEY_2, KEY_3 };
    for (int i = 0; i < 3; i++) {
        if (!Input::GetKeyDown(keys[i])) continue;
        rows[i].running = rows[i].timer.loop ? !rows[i].running : true;
        rows[i].timer.Reset();
    }

    // タイマーを進める(dt は倍率の影響を受けるので、スローだと遅くなり、ポーズ中は止まる)
    for (TimerRow& row : rows) {
        if (!row.running) continue;
        if (row.timer.Update(dt)) {
            row.firedCount++;
            if (!row.timer.loop) row.running = false;
        }
    }
}

void TimeDemoController::Draw() {
    // 時間の値(毎フレームの値。倍率を変えると dt だけが変わる)
    wchar_t buf[128];
    float y = infoY;
    swprintf_s(buf, L"GetDeltaTime()           %.4f 秒", Time::GetDeltaTime());
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(21.f));
    y -= infoGap;
    swprintf_s(buf, L"GetUnscaledDeltaTime()   %.4f 秒", Time::GetUnscaledDeltaTime());
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(21.f));
    y -= infoGap;
    swprintf_s(buf, L"GetTime()                %.2f 秒", Time::GetTime());
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(21.f));
    y -= infoGap;
    swprintf_s(buf, L"GetTimeScale() %.1f   IsPaused() %s", Time::GetTimeScale(), Time::IsPaused() ? L"true" : L"false");
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(21.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
    y -= infoGap;
    const wchar_t* state = paused ? L"ポーズ中(Pで再開)" : (slow ? L"スロー中(SPACEで戻す)" : L"通常の速さ");
    Text::Draw(state, infoX, y, MakeDemoTextStyle(21.f, paused ? DirectX::XMFLOAT4{ 1.0f, 0.5f, 0.5f, 1.0f } : DirectX::XMFLOAT4{ 0.7f, 1.0f, 0.7f, 1.0f }));

    // 回る2つの物の説明(物の下)
    const DirectX::XMFLOAT3 scaledPos = scaledObject->transform.position;
    const DirectX::XMFLOAT3 unscaledPos = unscaledObject->transform.position;
    Text::Draw(L"dt で回る", scaledPos.x, scaledPos.y - 75.f, MakeDemoTextStyle(19.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    Text::Draw(L"(スローで遅く、ポーズで止まる)", scaledPos.x, scaledPos.y - 100.f, MakeDemoTextStyle(16.f, { 0.8f, 0.8f, 0.8f, 1.0f }, TextAlign::Center));
    Text::Draw(L"UnscaledDeltaTime で回る", unscaledPos.x, unscaledPos.y - 75.f, MakeDemoTextStyle(19.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    Text::Draw(L"(いつも同じ速さ)", unscaledPos.x, unscaledPos.y - 100.f, MakeDemoTextStyle(16.f, { 0.8f, 0.8f, 0.8f, 1.0f }, TextAlign::Center));

    // タイマー: 説明・進みの帯・時間が来た回数
    for (int i = 0; i < 3; i++) {
        const TimerRow& row = rows[i];
        const float rowY = timerY - timerGap * (float)i;
        Text::Draw(row.label, timerX, rowY, MakeDemoTextStyle(20.f, { 0.6f, 0.9f, 1.0f, 1.0f }));

        // 帯(背景 → 進んだ分)。数えていない間は空
        const float progress = row.running ? row.timer.GetProgress() : 0.f;
        const float barY = rowY - 26.f;
        Image::DrawRect({ timerX + barWidth * 0.5f, barY }, { barWidth, barHeight }, { 0.0f, 0.0f, 0.0f, 0.6f });
        if (progress > 0.f) {
            Image::DrawRect({ timerX + barWidth * progress * 0.5f, barY }, { barWidth * progress, barHeight }, { 0.4f, 0.9f, 0.4f, 1.0f });
        }

        swprintf_s(buf, L"来た回数 %d", row.firedCount);
        Text::Draw(buf, timerX + barWidth + 14.f, barY, MakeDemoTextStyle(20.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
    }
}

void TimeDemoController::Uninit() {
    // 時間の倍率は全体の設定。残すと次の画面が遅い・止まったままになるので、必ず1に戻す
    Time::SetTimeScale(1.f);
}
