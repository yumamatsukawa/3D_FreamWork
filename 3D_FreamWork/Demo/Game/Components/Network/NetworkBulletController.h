#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// 弾の動き(ホストだけが動かす。クライアントは位置の同期で動く)。一定時間たったら Network::Despawn で全員の画面から消す
class NetworkBulletController : public Component {
private:
    DirectX::XMFLOAT3 velocity{ 0.f, 0.f, 0.f };   // 動く向きと速さ
    float life = 0.f;                               // 残りの時間(秒)

public:
    static constexpr float lifetime = 1.8f;         // 弾が飛んでいる時間
    static constexpr float speed = 350.f;           // 弾の速さ

    // 弾を出す時(ObjectPoolで借りた直後に、毎回呼ぶ。前の値が残っているので全部付け直す)
    void Launch();

    void Update(float dt) override;
};