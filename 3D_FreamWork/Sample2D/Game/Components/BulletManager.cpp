#include "BulletManager.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/MathUtil.h"
#include "../../../Engine/Audio.h"
#include "BulletController.h"

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

        // 撃った本人の正面(rotate.z = 0 で上向き)へ
        Fire(shooter->transform.position, Math::Forward2D(shooter->transform));
    });
}

GameObject* BulletManager::Fire(XMFLOAT3 position, XMFLOAT2 direction) {
    // プールから借りて、位置・向きをリセット(前回使った時の値が残っているため)
    GameObject* bullet = pool.Rent(*GetOwner()->GetScene(), SetupBullet);
    bullet->transform.position = position;
    bullet->transform.rotate = { 0.f, 0.f, 0.f };

    // 発射音(その場所で鳴らす。連射しても重なって鳴る)
    Audio::PlayOneShotAt(L"Assets/Sounds/shot.wav", position);

    // 進む方向と、消える時の返却先を渡す
    BulletController* controller = bullet->GetComponent<BulletController>();
    if (controller) controller->Fire(direction, &pool);

    return bullet;
}
