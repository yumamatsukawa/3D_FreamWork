#include "PlayerController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/Collider.h"
#include "../../../Engine/Image.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"

void PlayerController::Update(float dt) {
    Transform& t = GetOwner()->transform;
    SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>();
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();

    if (invincibleTimer > 0.f) invincibleTimer -= dt;

    // ★ このフレームの物理更新(Physics::Update)で既にOnCollisionStay2Dが呼ばれていれば
    //   touchingEnemyはtrueになっている。trueの時は白へ戻さず、赤のままにする
    if (renderer && !touchingEnemy) renderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    touchingEnemy = false;

    DirectX::XMFLOAT2 mousePos = Input::GetMousePosition();
    // ★ マウス座標はOS標準のY+=下方向のまま(Input::GetMousePositionは画面中心基準にするだけ)。
    //   ワールド座標はY+=上方向なので、比較する前に符号を反転させる
    DirectX::XMFLOAT2 mouseWorldPos = { mousePos.x, -mousePos.y };
    if (Input::GetKeyPress(MOUSE_LEFT)) {
        if (IsPointInBox(mouseWorldPos, t.GetWorldTransform())) {
            if (renderer) renderer->SetColor(1.0f, 0.0f, 0.0f, 0.5f);
        }
    }

    // 移動処理: 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す。
    //   実際に動く・何かにぶつかって止まる、はPhysX自身の計算に任せる
    DirectX::XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
    if (Input::GetKeyPress(KEY_W)) velocity.y += speed;
    if (Input::GetKeyPress(KEY_S)) velocity.y -= speed;
    if (Input::GetKeyPress(KEY_A)) velocity.x -= speed;
    if (Input::GetKeyPress(KEY_D)) velocity.x += speed;
    if (rigidbody) rigidbody->SetVelocity(velocity);

    // 見た目の向き(Q/E)は物理の回転とは無関係に、Transformを直接操作する
    // (弾の発射方向にもそのまま使われる。GameScene.cppのFireBullet参照)
    if (Input::GetKeyPress(KEY_Q)) t.rotate.z += speed * dt;
    if (Input::GetKeyPress(KEY_E)) t.rotate.z -= speed * dt;

    // SPACEキーで「撃った」イベントを発行するだけ。
    // 実際に弾を生成する処理は知らない(GameScene側がFireBulletを購読して行う)
    if (Input::GetKeyDown(KEY_SPACE)) {
        EventBus::Get().PublishObject("FireBullet", GetOwner());
    }

    // カメラをプレイヤーに追従させる(トップダウンのシンプルな追従カメラ)
    Image::GetCamera().position.x = t.position.x;
    Image::GetCamera().position.y = t.position.y;
}

void PlayerController::OnCollisionStay2D(CollisionInfo info) {
    if (info.other->GetTag() == "Enemy") {
        touchingEnemy = true;
        SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>();
        if (renderer) renderer->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
        // OnCollisionStay2Dは接触中ずっと毎フレーム呼ばれるので、無敵時間で連続ダメージを防ぐ
        if (hp <= 0 || invincibleTimer > 0.f) return;
        hp--;
        invincibleTimer = invincibleDuration;
        // ゲームオーバーにするかどうかはGameManagerの仕事なので、ここでは知らせるだけ
        if (hp <= 0) EventBus::Get().Publish("PlayerDied");
    }
}
