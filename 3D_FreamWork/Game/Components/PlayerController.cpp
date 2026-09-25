#include "PlayerController.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/Input.h"
#include "../../Engine/Collider.h"
#include "../../Engine/Image.h"
#include "../../Engine/EventBus.h"
#include "../../Engine/RigidbodyComponent.h"

void PlayerController::Update(float dt) {
    GameObject* owner = GetOwner();
    Transform& t = owner->transform;
    SpriteRenderer* renderer = owner->GetComponent<SpriteRenderer>();

    if (renderer) renderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

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
    if (Input::GetKeyPress(KEY_W)) velocity.z += speed;
    if (Input::GetKeyPress(KEY_S)) velocity.z -= speed;
    if (Input::GetKeyPress(KEY_A)) velocity.x -= speed;
    if (Input::GetKeyPress(KEY_D)) velocity.x += speed;

    // 基底クラス(RigidbodyComponent)で探すことで、PlayerがBox/Sphereどちらの
    // 形状でも(Box/SphereRigidbodyComponentのどちらが付いていても)動くようにしてある
    RigidbodyComponent* rigidbody = owner->GetComponent<RigidbodyComponent>();
    if (rigidbody) {
        // ★ Y速度はここで0を渡さず、今の値をそのまま維持する。
        //   毎フレーム0を渡すと、重力(SetUseGravity)で付いたY速度がすぐ消されてしまい、
        //   自由落下がほとんど効かなくなる(SetFreezePositionY(true)の時はどちらでも
        //   結果は同じだが、重力を使う設定に変えた時のために正しくしておく)
        velocity.y = rigidbody->GetVelocity().y;
        rigidbody->SetVelocity(velocity);
    }

    // 見た目の向き(Q/E)は物理の回転とは無関係に、Transformを直接操作する
    // (弾の発射方向にもそのまま使われる。GameScene.cppのFireBullet参照)
    if (Input::GetKeyPress(KEY_Q)) t.rotate.z += speed * dt;
    if (Input::GetKeyPress(KEY_E)) t.rotate.z -= speed * dt;

    // SPACEキーで「撃った」イベントを発行するだけ。
    // 実際に弾を生成する処理は知らない(GameScene側がFireBulletを購読して行う)
    if (Input::GetKeyDown(KEY_SPACE)) {
        EventBus::Get().PublishObject("FireBullet", owner);
    }

    // カメラをプレイヤーに追従させる
    Image::GetCamera().position.x = t.position.x;
    Image::GetCamera().position.y = t.position.y;
}

void PlayerController::OnCollisionStay2D(CollisionInfo info) {
    if (info.other->GetTag() == "Enemy") {
        SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>();
        if (renderer) renderer->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
    }
}
