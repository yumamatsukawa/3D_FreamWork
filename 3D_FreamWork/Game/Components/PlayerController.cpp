#include "PlayerController.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/Input.h"
#include "../../Engine/Collider.h"
#include "../../Engine/Image.h"

void PlayerController::Update(float dt) {
    GameObject* owner = GetOwner();
    Transform& t = owner->transform;
    SpriteRenderer* renderer = owner->GetComponent<SpriteRenderer>();

    if (renderer) renderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

    DirectX::XMFLOAT2 mousePos = Input::GetMousePosition();
    if (Input::GetKeyPress(MOUSE_LEFT)) {
        if (IsPointInBox(mousePos, t.GetWorldTransform())) {
            if (renderer) renderer->SetColor(1.0f, 0.0f, 0.0f, 0.5f);
        }
    }

    // 移動処理
    if (Input::GetKeyPress(KEY_W)) t.position.y -= speed * dt;
    if (Input::GetKeyPress(KEY_S)) t.position.y += speed * dt;
    if (Input::GetKeyPress(KEY_A)) t.position.x -= speed * dt;
    if (Input::GetKeyPress(KEY_D)) t.position.x += speed * dt;
    if (Input::GetKeyPress(KEY_Q)) t.rotate.z += speed * dt;
    if (Input::GetKeyPress(KEY_E)) t.rotate.z -= speed * dt;

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
