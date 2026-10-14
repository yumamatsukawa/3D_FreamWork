#include "EnemyManager.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/MathUtil.h"
#include "../../../Engine/ChaseComponent.h"
#include "EnemyController.h"

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

        // 動き(追いかける相手と速さはSpawnで渡す)
        obj->AddComponent<ChaseComponent>();

        // 弾に当たった時の処理
        obj->AddComponent<EnemyController>();
    }
}

void EnemyManager::Init() {
    // 最初の1体だけfirstSpawnDelay秒で出す(その分だけ経過時間を先に進めておく)
    spawnTimer.elapsed = spawnInterval - firstSpawnDelay;

    // Playerが倒れたら出現を止める
    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している
    EventBus::Get().Subscribe("PlayerDied", [this]() {
        stopped = true;
    });
}

void EnemyManager::Update(float dt) {
    if (stopped || !target) return;

    // 一定間隔で出現
    if (spawnTimer.Update(dt)) {
        // targetを中心に、ランダムな方向・一定距離離れた場所に出現させる
        XMFLOAT2 offset = Math::RandomOnCircle(spawnDistance);
        XMFLOAT3 pos = {
            target->transform.position.x + offset.x,
            target->transform.position.y + offset.y,
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
    SpriteRenderer* sprite = enemy->GetComponent<SpriteRenderer>();
    if (sprite) sprite->SetColor(1.0f, 0.0f, 1.0f, 1.0f);

    // 追いかける相手と速さを渡す
    ChaseComponent* chase = enemy->GetComponent<ChaseComponent>();
    if (chase) {
        chase->SetTarget(target);
        chase->SetSpeed(enemySpeed);
    }

    // 倒された時の返却先を渡す
    EnemyController* controller = enemy->GetComponent<EnemyController>();
    if (controller) controller->Spawn(&pool);

    return enemy;
}
