#include "DemoScreenFixController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/Image.h"

void DemoScreenFixController::Init() {
    // 付けた時の位置を、画面の位置として覚える
    screenPosition = { GetOwner()->transform.position.x, GetOwner()->transform.position.y };
}

void DemoScreenFixController::Place() {
    // 2Dカメラは、描く時に(物の位置 - カメラの位置)で画面に置く。カメラの分を足しておけば、画面の同じ場所に見える
    const Camera& camera = Image::GetCamera();
    GetOwner()->transform.position.x = screenPosition.x + camera.position.x;
    GetOwner()->transform.position.y = screenPosition.y + camera.position.y;
}

void DemoScreenFixController::Update(float dt) {
    Place();
}

void DemoScreenFixController::Draw() {
    Place();
}
