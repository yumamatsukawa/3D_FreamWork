#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <functional>

class GameObject;

// プールで使い回す弾の動き。Launch で位置・向き・寿命を決めて飛ばし、寿命が来たら「返して」と頼む(返すのはプールの持ち主)
class PoolBulletController : public Component {
private:
    DirectX::XMFLOAT2 velocity{ 0.f, 0.f };         // 飛ぶ速さ(1秒あたり)
    float remainingLife = 0.f;                      // 消えるまでの残り時間(秒)
    std::function<void(GameObject*)> onExpire;      // 寿命が来た時に呼ぶ処理(プールに返す)

public:
    explicit PoolBulletController(std::function<void(GameObject*)> expireAction) : onExpire(std::move(expireAction)) {}

    void Launch(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& speed, float lifeTime);   // 借りた弾を飛ばし始める

    void Update(float dt) override;
};
