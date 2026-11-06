#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// 箱を、そのプレイヤーの入力で動かす(ルールはホストだけ)。ホストは Network::GetInput で入力を読んで SetVelocity で動かす。
// どの画面でも、そのプレイヤーがつながっている間は箱を明るく、いない間は暗くする
class NetworkBoxController : public Component {
private:
    bool is3D;                         // 3Dか2Dか(3Dは前後に動いてジャンプできる)
    DirectX::XMFLOAT4 baseColor;       // つながっている時の色
    static constexpr float speed = 320.f;       // 移動の速さ
    static constexpr float jumpSpeed = 520.f;   // ジャンプの初速(3D)

public:
    NetworkBoxController(bool box3D, DirectX::XMFLOAT4 color) : is3D(box3D), baseColor(color) {}

    void Update(float dt) override;
};