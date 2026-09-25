#include "BulletManager.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include "../../Engine/RigidbodyComponent.h"
#include "../Components/BulletController.h"

namespace {
    // プールに新しい弾を追加する時に1回だけ呼ばれる組み立て処理
    void SetupBullet(GameObject* obj) {
        obj->transform.scale = { 16.0f, 16.0f, 1.0f };
        auto* meshRenderer = obj->AddComponent<MeshRenderer>(Mesh::CreateSphere());
        meshRenderer->SetTexture(L"Assets/grid.png");
        // 当たり判定+物理演算(PhysX): キネマティック、すり抜ける円形(命中判定用)
        obj->AddComponent<SphereRigidbodyComponent>(BodyType::Kinematic, 8.0f, 1.0f, /*isTrigger*/ true);
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
