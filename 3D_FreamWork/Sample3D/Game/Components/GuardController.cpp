#include "GuardController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/VisionComponent.h"
#include "../../../Engine/LightComponent.h"
#include "../../../Engine/MathUtil.h"
#include <cmath>

void GuardController::Init() {
    // 同じオブジェクトに付いている視野と見た目を探す(CreateGuard()で、このComponentより先に付けておく)
    vision = GetOwner()->GetComponent<VisionComponent>();
    renderer = GetOwner()->GetComponent<MeshRenderer>();
    torch = GetOwner()->GetComponent<LightComponent>();
}

void GuardController::Update(float dt) {
    // ゆっくり回る(視野の正面はrotate.yに合わせて動く)
    GetOwner()->transform.rotate.y += turnSpeed * dt;

    // 懐中電灯: 視野の正面(水平)を、少し下に傾けた向きへ照らす
    if (torch) {
        float pitch = XMConvertToRadians(torchPitch);
        XMFLOAT3 forward = Math::Forward3D(GetOwner()->transform);
        torch->SetDirection({ forward.x * cosf(pitch), -sinf(pitch), forward.z * cosf(pitch) });
    }

    // 視野に入っていて、壁にさえぎられていなければ見えた: 赤、見えていなければ青
    bool seen = vision && target && vision->CanSee(target);
    if (renderer) {
        if (seen) renderer->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
        else renderer->SetColor(0.3f, 0.5f, 1.0f, 1.0f);
    }
}