#include "EventScoreManager.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Core/GameObject.h"
#include <cstdio>

namespace {
    constexpr float columnX = -190.f;     // この欄の左端
    constexpr float columnY = 190.f;      // 欄の1行目(説明の板の下)
}

void EventScoreManager::Init() {
    // 撃破されたら得点を足して、得点が変わったことを知らせる(誰が撃破を送ったかは知らない)
    EventBus::Get().Subscribe("EnemyDefeated", [this]() {
        score += defeatScore;
        receivedCount++;
        EventBus::Get().Publish("ScoreChanged");
    });

    // コインが取られたら得点を足す。渡されたオブジェクトから、名前と位置を読める
    EventBus::Get().SubscribeObject("CoinPicked", [this](GameObject* coin) {
        score += coinScore;
        receivedCount++;
        wchar_t buf[64];
        swprintf_s(buf, L"%S (%.0f, %.0f)", coin->GetName().c_str(), coin->transform.position.x, coin->transform.position.y);
        lastCoinInfo = buf;
        EventBus::Get().Publish("ScoreChanged");
    });
}

void EventScoreManager::Draw() {
    Text::Draw(L"受け取る側 (EventScoreManager)", columnX, columnY, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
    Text::Draw(L"スコア: " + std::to_wstring(score), columnX, columnY - 42.f, MakeDemoTextStyle(36.f, { 0.7f, 1.0f, 0.7f, 1.0f }));
    Text::Draw(L"受け取った数: " + std::to_wstring(receivedCount), columnX, columnY - 82.f, MakeDemoTextStyle(22.f));
    Text::Draw(L"最後のコイン: " + lastCoinInfo, columnX, columnY - 112.f, MakeDemoTextStyle(19.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
}
