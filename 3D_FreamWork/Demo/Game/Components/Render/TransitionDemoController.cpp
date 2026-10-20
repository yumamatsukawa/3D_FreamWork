#include "TransitionDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../Scenes/Render/TransitionDemoScene.h"
#include "../../Scenes/Render/TransitionDemoBScene.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/SceneManager.h"
#include <cstdio>

namespace {
    // 演出の名前と、呼んでいる書き方
    struct TypeRow {
        const wchar_t* name;
        const wchar_t* call;
    };
    const TypeRow rows[4] = {
        { L"Fade(黒)",   L"FadeTransition()   黒で 0.5秒かけて暗転 → 切り替え → 0.5秒かけて明転" },
        { L"Fade(白)",   L"FadeTransition({1,1,1,1}, 1.0)   白で 1秒ずつ" },
        { L"丸",         L"SpriteTransition(\"transition_circle.png\", 8, 2, 0.04)   丸が閉じて開く(画像を逆再生)" },
        { L"ワイプ",     L"SpriteTransition(\"transition_wipe.png\", 8, 2, 0.04).SetSwitchFrame(7)   順再生だけで流す" },
    };

    // 文字の位置
    constexpr float listX = -610.f;       // 一覧の左端
    constexpr float listY = 190.f;        // 一覧の1行目(説明の板の下)
    constexpr float listGap = 36.f;       // 行の間隔
    constexpr float statusY = 36.f;       // 状態の1行目
    constexpr float statusGap = 30.f;     // 状態の行の間隔
    constexpr DirectX::XMFLOAT2 titlePos = { 360.f, 30.f };   // 今のシーンの名前(大きな文字)の中心

    // 演出type で、シーンTへ切り替える(ChangeSceneはテンプレートなので、シーンの型ごとにこの関数が作られる)
    template<typename T>
    void ChangeWith(int type) {
        SceneManager& manager = SceneManager::Get();
        switch (type) {
        case 0: manager.ChangeScene<T>(FadeTransition()); break;
        case 1: manager.ChangeScene<T>(FadeTransition({ 1.f, 1.f, 1.f, 1.f }, 1.f)); break;
        case 2: manager.ChangeScene<T>(SpriteTransition(L"Assets/transition_circle.png", 8, 2, 0.04f)); break;
        default: manager.ChangeScene<T>(SpriteTransition(L"Assets/transition_wipe.png", 8, 2, 0.04f).SetSwitchFrame(7)); break;
        }
    }
}

void TransitionDemoController::Start(int type) {
    if (page == TransitionPage::A) ChangeWith<TransitionDemoBScene>(type);
    else ChangeWith<TransitionDemoScene>(type);
}

void TransitionDemoController::Update(float dt) {
    // 経過時間(切り替え中はdtが0なので止まる)
    elapsed += dt;

    // 切り替え中は操作を受け付けない(SceneManagerも、切り替え中の頼みは無視する)
    if (Time::IsPaused()) return;

    // 1〜4: 演出を選んで、もう一方のシーンへ切り替える
    for (int i = 0; i < 4; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) Start(i);
    }
}

void TransitionDemoController::Draw() {
    // 演出の一覧
    for (int i = 0; i < 4; i++) {
        const float y = listY - listGap * (float)i;
        wchar_t buf[32];
        swprintf_s(buf, L"[%d] %ls", i + 1, rows[i].name);
        Text::Draw(buf, listX, y, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
        Text::Draw(rows[i].call, listX + 190.f, y, MakeDemoTextStyle(17.f, { 0.8f, 0.9f, 1.0f, 1.0f }));
    }

    // 切り替え中は、ゲームの時間が止まる(Time::IsPaused が true。経過時間も止まる)
    const bool paused = Time::IsPaused();
    Text::Draw(std::wstring(L"Time::IsPaused() = ") + (paused ? L"true(切り替え中。時間が止まる)" : L"false"),
        listX, statusY, MakeDemoTextStyle(20.f, paused ? DirectX::XMFLOAT4(1.0f, 0.6f, 0.4f, 1.0f) : DirectX::XMFLOAT4(0.7f, 1.0f, 0.7f, 1.0f)));
    wchar_t buf[96];
    swprintf_s(buf, L"このシーンに入ってからの時間 = %.1f 秒", elapsed);
    Text::Draw(buf, listX, statusY - statusGap, MakeDemoTextStyle(20.f));
    Text::Draw(L"Esc の戻る(黒のFade)も、同じ SceneManager の切り替えです", listX, statusY - statusGap * 2.f,
        MakeDemoTextStyle(17.f, { 0.8f, 0.8f, 0.8f, 1.0f }));

    // 今のシーンの名前(大きな文字。AかBかで、切り替わったことが分かる)
    const bool pageA = (page == TransitionPage::A);
    Text::Draw(pageA ? L"SCENE A" : L"SCENE B", titlePos.x, titlePos.y,
        MakeDemoTextStyle(56.f, pageA ? DirectX::XMFLOAT4(0.5f, 0.8f, 1.0f, 1.0f) : DirectX::XMFLOAT4(1.0f, 0.7f, 0.4f, 1.0f), TextAlign::Center));
    Text::Draw(pageA ? L"(2Dの背景)" : L"(3Dの舞台)", titlePos.x, titlePos.y - 50.f, MakeDemoTextStyle(22.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
}
