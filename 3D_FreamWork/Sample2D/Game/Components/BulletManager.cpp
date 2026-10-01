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
        obj->transform.scale = { 16.0f, 16.0f, 1.0f };

        // 見た目
        auto* meshRenderer = obj->AddComponent<MeshRenderer>(Mesh::CreateSphere());
        meshRenderer->SetTexture(L"Assets/grid.png");

        // 当たり判定(Kinematic・トリガー: 物理で押し合わず、当たったことだけ知らせる)
        obj->AddComponent<CircleRigidbodyComponent>(BodyType::Kinematic, 8.0f, 1.0f, true);

        // 動き
        obj->AddComponent<BulletController>();
    }
}

void BulletManager::Init() {
    // 誰かが弾を撃ったら(FireBullet)、撃った本人の向きへ発射
    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している
    EventBus::Get().SubscribeObject("FireBullet", [this](GameObject* shooter) {
        if (!shooter) return;

        // rotate.z = 0 の時の正面を(0, 1)として、見た目の回転と同じ向きに回転させる
        float rad = -shooter->transform.rotate.z * (DirectX::XM_PI / 180.f);
        XMFLOAT2 dir = { sinf(rad), cosf(rad) };
        Fire(shooter->transform.position, dir);
    });
}

GameObject* BulletManager::Fire(XMFLOAT3 position, XMFLOAT2 direction) {
    // プールから借りて、位置・向きをリセット(前回使った時の値が残っているため)
    GameObject* bullet = pool.Rent(*GetOwner()->GetScene(), SetupBullet);
    bullet->transform.position = position;
    bullet->transform.rotate = { 0.f, 0.f, 0.f };

    // 進む方向と、消える時の返却先を渡す
    BulletController* controller = bullet->GetComponent<BulletController>();
    if (controller) controller->Fire(direction, &pool);

    return bullet;
}
