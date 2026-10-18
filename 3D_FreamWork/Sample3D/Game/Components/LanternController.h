#pragma once
#include "../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// ランタンの動きを担当する(中心の周りをゆっくり回りながら、上下にふわふわ揺れる。光はLightComponentが担当)
class LanternController : public Component {
private:
    DirectX::XMFLOAT3 center{ 0.0f, 0.0f, 0.0f };   // 回る中心
    float angle = 0.0f;                              // 今の角度(度)
    float bobTimer = 0.0f;                           // 上下の揺れ用の時間(秒)

    static constexpr float orbitRadius = 650.0f;     // 回る半径
    static constexpr float orbitSpeed = 25.0f;       // 回る速さ(1秒に何度)
    static constexpr float baseHeight = 120.0f;      // 基準の高さ
    static constexpr float bobHeight = 40.0f;        // 上下に揺れる幅
    static constexpr float bobSpeed = 2.0f;          // 上下の揺れの速さ

public:
    // 回る中心を、CreateLantern()から渡してもらう
    void Setup(DirectX::XMFLOAT3 centerPosition) { center = centerPosition; }

    void Update(float dt) override;
};
