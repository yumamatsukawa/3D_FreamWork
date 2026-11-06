#pragma once
#include "../../../../Engine/Core/Component.h"

// 自分のキー入力を集めて、Network に渡す(ネットワークのデモ共通)。自分の箱(所有者が自分)に付けた分だけが働く。
// ホストなら自分の入力がそのまま GetInput になり、クライアントならホストへ送られる。オフラインでも同じに動く
class NetworkPlayerInputController : public Component {
public:
    static constexpr int jumpButton = 0;   // ボタン0 = ジャンプ(3Dのデモ)

    void Update(float dt) override;
};