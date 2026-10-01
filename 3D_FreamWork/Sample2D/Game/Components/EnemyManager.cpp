#include "EnemyManager.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/EventBus.h"
#include "EnemyController.h"
#include <cstdlib>
#include <cmath>

namespace {
    // プールに新しい敵を追加する時に1回だけ呼ばれる組み立て処理
    void SetupEnemy(GameObject* obj) {
        obj->SetTag("Enemy");
        obj->transform.scale = { 100.0f, 100.0f, 100.0f };

        // 見た目
        auto* sprite = obj->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
        sprite->SetSpriteIndex(5);
        sprite->SetColor(1.0f, 0.0f, 1.0f, 1.0f);

        // 当たり判定+物理演算(PhysX)
        auto* rigidbody = obj->AddComponent<SquareRigidbodyComponent>(BodyType::Dynamic, 100.0f, 1.0f);
        rigidbody->SetUseGravity(false);
        rigidbody->SetFreezeRotation(true);
        rigidbody->SetFreezePositionZ(true);

        // 動き
        obj->AddComponent<EnemyController>();
    }
}

void EnemyManager::Init() {
    // Playerが倒れたら出現を止める
    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している
    EventBus::Get().Subscribe("PlayerDied", [this]() {
        stopped = true;
    });
}

void EnemyManager::Update(float dt) {
    if (stopped || !target) return;

    // 一定間隔で出現
    spawnTimer -= dt;
    if (spawnTimer <= 0.f) {
        spawnTimer = spawnInterval;

        // targetを中心に、ランダムな方向・一定距離離れた場所に出現させる
        float angle = ((float)rand() / RAND_MAX) * DirectX::XM_2PI;
        XMFLOAT3 pos = {
            target->transform.position.x + cosf(angle) * spawnDistance,
            target->transform.position.y + sinf(angle) * spawnDistance,
            0.0f
        };
        Spawn(pos);
    }
}

GameObject* EnemyManager::Spawn(XMFLOAT3 position) {
    // プールから借りて、位置・向き・色をリセット(前回使った時の値が残っているため)
    GameObject* enemy = pool.Rent(*GetOwner()->GetScene(), SetupEnemy);
    enemy->transform.position = position;
    enemy->transform.rotate = { 0.f, 0.f, 0.f };
    enemy->GetComponent<SpriteRenderer>()->SetColor(1.0f, 0.0f, 1.0f, 1.0f);

    // 追いかける相手と、倒された時の返却先を渡す
    EnemyController* controller = enemy->GetComponent<EnemyController>();
    if (controller) controller->Spawn(target, &pool);

    return enemy;
}
