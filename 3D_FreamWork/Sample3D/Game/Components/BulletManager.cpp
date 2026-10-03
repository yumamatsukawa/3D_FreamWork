#include "BulletManager.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/EventBus.h"
#include "BulletController.h"
#include <cmath>

namespace {
    // プールに新しい弾を追加する時に1回だけ呼ばれる組み立て処理
    void SetupBullet(GameObject* obj) {
        obj->SetTag("Bullet");
        obj->transform.scale = { 20.0f, 20.0f, 20.0f };

        // 見た目
        auto* meshRenderer = obj->AddComponent<MeshRenderer>(Mesh::CreateSphere());
        meshRenderer->SetTexture(L"Assets/grid.png");

        // 当たり判定(Kinematic・トリガー: 物理で押し合わず、当たったことだけ知らせる)
        obj->AddComponent<SphereRigidbodyComponent>(BodyType::Kinematic, 10.0f, 1.0f, true);

        // 動き
        obj->AddComponent<BulletController>();
    }
}

void BulletManager::Init() {
    // 誰かが弾を撃ったら(FireBullet)、撃った本人の向いている方向へ発射
    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している
    EventBus::Get().SubscribeObject("FireBullet", [this](GameObject* shooter) {
        if (!shooter) return;

        // rotate.y(度)の時の正面は (sin, 0, cos)。Yは0にして地面と水平に飛ばす
        float rad = DirectX::XMConvertToRadians(shooter->transform.rotate.y);
        XMFLOAT3 dir = { sinf(rad), 0.f, cosf(rad) };

        // 撃った本人の中心から、少し前に出す(本人の体の中に埋もれて見えないため)
        XMFLOAT3 pos = shooter->transform.position;
        pos.x += dir.x * muzzleDistance;
        pos.z += dir.z * muzzleDistance;
        Fire(pos, dir);
    });
}

GameObject* BulletManager::Fire(XMFLOAT3 position, XMFLOAT3 direction) {
    // プールから借りて、位置・向きをリセット(前回使った時の値が残っているため)
    GameObject* bullet = pool.Rent(*GetOwner()->GetScene(), SetupBullet);
    bullet->transform.position = position;
    bullet->transform.rotate = { 0.f, 0.f, 0.f };

    // 進む方向と、消える時の返却先を渡す
    BulletController* controller = bullet->GetComponent<BulletController>();
    if (controller) controller->Fire(direction, &pool);

    return bullet;
}
