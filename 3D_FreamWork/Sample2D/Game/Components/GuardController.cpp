#include "GuardController.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render/SpriteRenderer.h"
#include "../../../Engine/Behavior/VisionComponent.h"

void GuardController::Init() {
    // 同じオブジェクトに付いている視野と見た目を探す(CreateGuard()で、このComponentより先に付けておく)
    vision = GetOwner()->GetComponent<VisionComponent>();
    sprite = GetOwner()->GetComponent<SpriteRenderer>();
}

void GuardController::Update(float dt) {
    // ゆっくり回る(視野の正面はrotate.zに合わせて動く)
    GetOwner()->transform.rotate.z += turnSpeed * dt;

    // 視野に入っていて、壁にさえぎられていなければ見えた: 赤、見えていなければ青
    bool seen = vision && target && vision->CanSee(target);
    if (sprite) {
        if (seen) sprite->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
        else sprite->SetColor(0.3f, 0.5f, 1.0f, 1.0f);
    }
}