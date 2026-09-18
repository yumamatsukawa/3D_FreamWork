#include "BulletManager.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/ColliderComponent.h"
#include "../Components/BulletController.h"

namespace {
    // プールに新しい弾を追加する時に1回だけ呼ばれる組み立て処理
    void SetupBullet(GameObject* obj) {
        obj->transform.scale = { 16.0f, 16.0f, 1.0f };
        obj->AddComponent<SpriteRenderer>(L"Assets/box.png",3,3);
        obj->AddComponent<CircleColliderComponent>(true, 8.0f);  // すり抜ける円形(命中判定用)
        obj->AddComponent<BulletController>();
    }
}

GameObject* BulletManager::Fire(Scene& scene, XMFLOAT3 position, XMFLOAT2 direction) {
    GameObject* bullet = pool.Rent(scene, SetupBullet);
    bullet->transform.position = position;
    bullet->transform.rotate = { 0.f, 0.f, 0.f };

    BulletController* controller = bullet->GetComponent<BulletController>();
    if (controller) controller->Fire(direction, &pool);

    return bullet;
}
