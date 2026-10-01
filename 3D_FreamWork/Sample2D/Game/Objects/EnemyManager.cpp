#include "EnemyManager.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../Components/EnemyController.h"

namespace {
    // プールに新しい敵を追加する時に1回だけ呼ばれる組み立て処理
    void SetupEnemy(GameObject* obj) {
        obj->SetTag("Enemy");
        obj->transform.scale = { 100.0f, 100.0f, 100.0f };

        auto* sprite = obj->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
        sprite->SetSpriteIndex(5);
        sprite->SetColor(1.0f, 0.0f, 1.0f, 1.0f);

        // 当たり判定+物理演算(PhysX)。Playerと同じく、重力なし・回転なし・奥行き方向に動かない
        auto* rigidbody = obj->AddComponent<SquareRigidbodyComponent>(BodyType::Dynamic, 100.0f, 1.0f);
        rigidbody->SetUseGravity(false);
        rigidbody->SetFreezeRotation(true);
        rigidbody->SetFreezePositionZ(true);

        obj->AddComponent<EnemyController>();
    }
}

GameObject* EnemyManager::Spawn(Scene& scene, XMFLOAT3 position, GameObject* target) {
    GameObject* enemy = pool.Rent(scene, SetupEnemy);
    enemy->transform.position = position;
    enemy->transform.rotate = { 0.f, 0.f, 0.f };
    enemy->GetComponent<SpriteRenderer>()->SetColor(1.0f, 0.0f, 1.0f, 1.0f);  // 再利用時に色が残らないよう戻す

    EnemyController* controller = enemy->GetComponent<EnemyController>();
    if (controller) controller->Spawn(target, &pool);

    return enemy;
}
