#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

// EventBusデモの「受け取る側」(スコア)。イベントを聞いて得点を足し、"ScoreChanged" を送る。送る側のことは知らない。
// 登録したラムダは this を使うので、シーンの Uninit で EventBus::Get().Clear() する
class EventScoreManager : public Component {
private:
    static constexpr int defeatScore = 100;    // 撃破1回の得点
    static constexpr int coinScore = 10;       // コイン1枚の得点

    int score = 0;                             // 今の得点
    int receivedCount = 0;                     // 受け取ったイベントの数
    std::wstring lastCoinInfo = L"-";          // 最後に受け取ったコインの情報(渡されたオブジェクトから読む)

public:
    void Init() override;
    void Draw() override;
};
