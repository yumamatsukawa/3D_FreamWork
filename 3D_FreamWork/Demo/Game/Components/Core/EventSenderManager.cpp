#include "EventSenderManager.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"

namespace {
    constexpr float columnX = -610.f;     // この欄の左端
    constexpr float columnY = 190.f;      // 欄の1行目(説明の板の下)
}

void EventSenderManager::SendDefeat() {
    // 誰が受け取るかは知らずに、名前だけで知らせる
    EventBus::Get().Publish("EnemyDefeated");
    sentCount++;
}

void EventSenderManager::SendCoin() {
    // 「誰が」を伝えたい時は、オブジェクトを1つ渡す
    EventBus::Get().PublishObject("CoinPicked", coin);
    sentCount++;
}

void EventSenderManager::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    if (Input::GetKeyDown(KEY_1)) SendDefeat();
    if (Input::GetKeyDown(KEY_2)) SendCoin();
}

void EventSenderManager::Draw() {
    Text::Draw(L"送る側 (EventSenderManager)", columnX, columnY, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
    Text::Draw(L"送った数: " + std::to_wstring(sentCount), columnX, columnY - 30.f, MakeDemoTextStyle(22.f));
    Text::Draw(L"Publish で名前だけを知らせる", columnX, columnY - 60.f, MakeDemoTextStyle(18.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
}
