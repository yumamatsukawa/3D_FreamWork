#pragma once
#include "../../../../Engine/Core/Component.h"

class GameObject;

// EventBusデモの「送る側」。「撃破」「コイン取得」のイベントを送るだけで、誰が受け取るかは知らない(キー1・2と、画面のボタンから呼ばれる)
class EventSenderManager : public Component {
private:
    GameObject* coin;       // コイン取得イベントで「誰が」として渡すオブジェクト
    int sentCount = 0;      // 送ったイベントの数

public:
    explicit EventSenderManager(GameObject* coinObject) : coin(coinObject) {}

    void SendDefeat();      // 引数なしイベント "EnemyDefeated" を送る
    void SendCoin();        // オブジェクトを1つ渡すイベント "CoinPicked" を送る

    void Update(float dt) override;
    void Draw() override;
};
