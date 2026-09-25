#include "Enemy.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/RigidbodyComponent.h"

GameObject* CreateEnemy(Scene& scene) {
    GameObject* enemy = scene.CreateObject("Enemy", "Enemy");
    enemy->transform.position = { 150.0f,   0.0f, 0.0f };
    enemy->transform.scale    = { 100.0f, 100.0f, 1.0f };

    // 見た目(Enemyは毎フレームの処理が無いので、SpriteRendererの設定だけで表現する)
    auto* sprite = enemy->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);
    sprite->SetWorldSpace(true);
    // X/Zの傾きは固定し、水平方向(Y軸)だけカメラに向く「立て看板」ビルボードにする
    sprite->SetColor(1.0f, 0.0f, 1.0f, 1.0f);

    // 当たり判定+物理演算(PhysX): キネマティック、すり抜けない四角形。
    // isStaticForPush=trueなので、Playerとぶつかった時にEnemy自身は押し戻されない
    // (Playerだけが押し戻される。旧・2D当たり判定でのisStatic=trueと同じ意味)。
    enemy->AddComponent<BoxRigidbodyComponent>(BodyType::Kinematic, enemy->transform.scale, 1.0f,
        /*isTrigger*/ false, /*isStaticForPush*/ true);

    return enemy;
}
