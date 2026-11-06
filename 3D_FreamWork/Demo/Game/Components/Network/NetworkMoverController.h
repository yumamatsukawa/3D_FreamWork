#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// 円を描いて動く(ホストだけが動かす。クライアントは位置の同期で動く)。回転も一緒に変えて、角度の補間も見せる
class NetworkMoverController : public Component {
private:
    bool is3D;                          // 3Dは地面と水平(XZ)に、2Dは画面(XY)に回る
    DirectX::XMFLOAT3 center;           // 円の中心
    float radius;                       // 円の半径
    float angle = 0.f;                  // 今の角度(ラジアン)
    static constexpr float angularSpeed = 1.2f;   // 回る速さ(ラジアン/秒)

public:
    NetworkMoverController(bool mover3D, DirectX::XMFLOAT3 circleCenter, float circleRadius)
        : is3D(mover3D), center(circleCenter), radius(circleRadius) {}

    void Update(float dt) override;
};