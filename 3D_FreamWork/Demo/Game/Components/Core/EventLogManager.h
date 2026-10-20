#pragma once
#include "../../../../Engine/Core/Component.h"
#include <deque>
#include <string>

// EventBusデモの「記録係」。流れてくるイベントをすべて聞いて、受け取った順に一覧にする。
// 登録したラムダは this を使うので、シーンの Uninit で EventBus::Get().Clear() する
class EventLogManager : public Component {
private:
    static constexpr size_t maxLines = 9;     // 残しておく行の数

    std::deque<std::wstring> lines;           // 新しい物が後ろ

    void AddLine(const std::wstring& text);   // 受け取った時刻を付けて、一覧に足す

public:
    void Init() override;
    void Draw() override;
};
