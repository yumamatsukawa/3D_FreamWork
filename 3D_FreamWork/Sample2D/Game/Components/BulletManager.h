#pragma once
#include <DirectXMath.h>
#include "../../../Engine/Component.h"
#include "../../../Engine/ObjectPool.h"

class GameObject;

// 弾の発射を担当する("FireBullet"を受けて、撃った本人の向きへ弾を出す)
class BulletManager : public Component {
private:
    ObjectPool pool;    // 弾のプール

public:
    void Init() override;

    // 指定位置・方向に弾を1発発射する
    GameObject* Fire(XMFLOAT3 position, XMFLOAT2 direction);
};
