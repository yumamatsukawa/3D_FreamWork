#include "GuardController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/VisionComponent.h"

void GuardController::Init() {
    // 同じオブジェクトに付いている視野と見た目を探す(CreateGuard()で、このComponentより先に付けておく)
    vision = GetOwner()->GetComponent<VisionComponent>();
    renderer = GetOwner()->GetComponent<MeshRenderer>();
}

void GuardController::Update(float dt) {
    // ゆっくり回る(視野の正面はrotate.yに合わせて動く)
    GetOwner()->transform.rotate.y += turnSpeed * dt;

    // 視野に入っていて、壁にさえぎられていなければ見えた: 赤、見えていなければ青
    bool seen = vision && target && vision->CanSee(target);
    if (renderer) {
        if (seen) renderer->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
        else renderer->SetColor(0.3f, 0.5f, 1.0f, 1.0f);
    }
}