#include "GuardController.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render3D/MeshRenderer.h"
#include "../../../Engine/Behavior/VisionComponent.h"
#include "../../../Engine/Render3D/LightComponent.h"
#include "../../../Engine/Core/MathUtil.h"
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

    // 視野に入っていて、壁にさえぎられていなければ見えた: 赤(太い赤の枠線)、見えていなければ青(細い青の枠線)。
    // 枠線は壁越しでも見える(seeThrough)ので、壁の向こうにいる見張り役の場所が分かる
    bool seen = vision && target && vision->CanSee(target);
    if (renderer) {
        if (seen) renderer->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
        else renderer->SetColor(0.3f, 0.5f, 1.0f, 1.0f);
        if (seen) renderer->SetOutline({ 1.0f, 0.1f, 0.1f, 1.0f }, outlineWidth, true);
        else renderer->SetOutline({ 0.3f, 0.6f, 1.0f, 1.0f }, calmOutlineWidth, true);
    }
}