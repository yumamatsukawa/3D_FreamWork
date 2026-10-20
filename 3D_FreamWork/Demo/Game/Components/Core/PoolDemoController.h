#pragma once
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Core/ObjectPool.h"
#include <DirectXMath.h>

// Poolデモの操作役。クリック・SPACEで弾をプールから借りて飛ばし、「借りた回数・使用中・待機中・作った数」を見せる。
// ObjectPool はこのComponentのメンバとして持つ(Sceneが消えれば一緒に消える)
class PoolDemoController : public Component {
private:
    static constexpr int prewarmCount = 10;                       // 最初に用意しておく弾の数
    static constexpr float fireInterval = 0.08f;                  // 弾を出す間隔(秒)
    static constexpr float bulletSpeed = 520.f;                   // 弾の速さ
    static constexpr float bulletLife = 1.6f;                     // 弾の寿命(秒)
    static constexpr DirectX::XMFLOAT3 muzzle = { 0.f, -200.f, 0.f };   // 弾を出す場所

    ObjectPool bulletPool;            // 弾のプール
    float cooldown = 0.f;             // 次に弾を出せるまでの時間
    int rentCount = 0;                // 弾を借りた回数

    void SetupBullet(GameObject* bullet);   // 弾を新しく作る時の組み立て(見た目と動き。最初の1回だけ呼ばれる)
    void Fire();                            // 弾を1つ借りて、マウスの方へ飛ばす
    int CountActive() const;                // 使用中の弾の数

public:
    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
