#include "ScreenEffectDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/ScreenEffect.h"
#include <cstdio>

namespace {
    // 演出1つ分の表示(キー・名前・呼んでいる関数)
    struct EffectRow {
        const wchar_t* key;
        const wchar_t* name;
        const wchar_t* call;
    };
    const EffectRow rows[4] = {
        { L"1", L"Flash(白い光)",      L"ScreenEffect::Flash({1,1,1,0.8}, 0.3秒)   光ってだんだん消える" },
        { L"2", L"Vignette(赤い縁)",   L"ScreenEffect::Vignette({1,0,0,0.7}, 1.2秒, 3回)   画面の端だけが点滅する" },
        { L"3", L"Fade(黒)",           L"ScreenEffect::Fade({0,0,0,1}, 1, 0.6秒, 明転する関数)   暗転 → 終わったら明転" },
        { L"4", L"Fade(白)",           L"ScreenEffect::Fade({1,1,1,1}, 1, 0.6秒, 明転する関数)   白く暗転 → 終わったら明転" },
    };

    // 文字の位置
    constexpr float listX = -610.f;       // 一覧の左端
    constexpr float listY = 190.f;        // 一覧の1行目(説明の板の下)
    constexpr float listGap = 38.f;       // 行の間隔
    constexpr float statusY = 20.f;       // 状態の1行目
    constexpr float statusGap = 34.f;     // 状態の行の間隔
    constexpr float fadeTime = 0.6f;      // Fade で暗くなる・明るくなる時間(秒)
}

void ScreenEffectDemoController::StartFade(DirectX::XMFLOAT4 color) {
    // 暗転(濃さ1)し終わったら onComplete が1回呼ばれるので、そこで明転(濃さ0)を始める。
    // この関数は、シーンが切り替わった時は捨てられる(消えたControllerを触らない)
    ScreenEffect::Fade(color, 1.f, fadeTime, [this, color] {
        completeCount++;
        ScreenEffect::Fade(color, 0.f, fadeTime);
    });
}

void ScreenEffectDemoController::Update(float dt) {
    // 光らせる時間を進める
    if (highlightTimer > 0.f) highlightTimer -= dt;

    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1〜4: 演出を出す
    if (Input::GetKeyDown(KEY_1)) {
        ScreenEffect::Flash({ 1.f, 1.f, 1.f, 0.8f }, 0.3f);
        lastPressed = 0;
    }
    else if (Input::GetKeyDown(KEY_2)) {
        ScreenEffect::Vignette({ 1.f, 0.f, 0.f, 0.7f }, 1.2f, 3);
        lastPressed = 1;
    }
    else if (Input::GetKeyDown(KEY_3)) {
        StartFade({ 0.f, 0.f, 0.f, 1.f });
        lastPressed = 2;
    }
    else if (Input::GetKeyDown(KEY_4)) {
        StartFade({ 1.f, 1.f, 1.f, 1.f });
        lastPressed = 3;
    }
    else {
        return;
    }
    highlightTimer = highlightTime;
}

void ScreenEffectDemoController::Draw() {
    // 演出の一覧(押した直後の行は黄色)
    for (int i = 0; i < 4; i++) {
        const bool lit = (i == lastPressed && highlightTimer > 0.f);
        const DirectX::XMFLOAT4 color = lit ? DirectX::XMFLOAT4(1.0f, 0.9f, 0.4f, 1.0f) : DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        const float y = listY - listGap * (float)i;
        Text::Draw(std::wstring(L"[") + rows[i].key + L"] " + rows[i].name, listX, y, MakeDemoTextStyle(24.f, color));
        Text::Draw(rows[i].call, listX + 270.f, y, MakeDemoTextStyle(18.f, { 0.8f, 0.9f, 1.0f, 1.0f }));
    }

    // 状態: IsFading は Fade の濃さが変わっている途中だけ true(暗転し終わって止まっている間は false)
    const bool fading = ScreenEffect::IsFading();
    Text::Draw(std::wstring(L"ScreenEffect::IsFading() = ") + (fading ? L"true(濃さが変わっている途中)" : L"false"),
        listX, statusY, MakeDemoTextStyle(22.f, fading ? DirectX::XMFLOAT4(1.0f, 0.6f, 0.4f, 1.0f) : DirectX::XMFLOAT4(0.7f, 1.0f, 0.7f, 1.0f)));
    wchar_t buf[96];
    swprintf_s(buf, L"Fade の完了で呼ばれた関数の回数 = %d", completeCount);
    Text::Draw(buf, listX, statusY - statusGap, MakeDemoTextStyle(22.f));
    Text::Draw(L"Flash と Vignette は同時に出せます(重なる順は 下から Vignette → Flash → Fade)",
        listX, statusY - statusGap * 2.f, MakeDemoTextStyle(19.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
    Text::Draw(L"文字も隠れる、画面の一番上への演出です。シーンを出る時に自動で元に戻ります",
        listX, statusY - statusGap * 3.f, MakeDemoTextStyle(19.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
}

void ScreenEffectDemoController::Uninit() {
    // 出る時に暗いまま(Fade は次のシーンへ残る)にならないよう、すぐ明るい状態に戻す
    ScreenEffect::Stop();
    ScreenEffect::Fade({ 0.f, 0.f, 0.f, 1.f }, 0.f, 0.f);
}
