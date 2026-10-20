#include "EventLogManager.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Time.h"
#include <cstdio>

namespace {
    constexpr float columnX = 240.f;      // この欄の左端
    constexpr float columnY = 190.f;      // 欄の1行目(説明の板の下)
    constexpr float lineGap = 28.f;       // 行の間隔
}

void EventLogManager::AddLine(const std::wstring& text) {
    wchar_t buf[16];
    swprintf_s(buf, L"[%.1f] ", Time::GetTime());
    lines.push_back(buf + text);
    if (lines.size() > maxLines) lines.pop_front();
}

void EventLogManager::Init() {
    // 流れてくるイベントを全部聞く(このComponentを先に作っておくと、ログの順番が「撃破 → 得点が変わった」になる)
    EventBus::Get().Subscribe("EnemyDefeated", [this]() { AddLine(L"EnemyDefeated を受信"); });
    EventBus::Get().SubscribeObject("CoinPicked", [this](GameObject* coin) {
        AddLine(L"CoinPicked を受信 (" + std::wstring(coin->GetName().begin(), coin->GetName().end()) + L")");
    });
    EventBus::Get().Subscribe("ScoreChanged", [this]() { AddLine(L"ScoreChanged を受信"); });
}

void EventLogManager::Draw() {
    Text::Draw(L"記録係 (EventLogManager)", columnX, columnY, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f }));

    // 新しい物ほど下(古い物は上へ流れていく)
    float y = columnY - 34.f;
    for (const std::wstring& line : lines) {
        Text::Draw(line, columnX, y, MakeDemoTextStyle(18.f));
        y -= lineGap;
    }
    if (lines.empty()) Text::Draw(L"(まだ何も受け取っていません)", columnX, y, MakeDemoTextStyle(18.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
}
