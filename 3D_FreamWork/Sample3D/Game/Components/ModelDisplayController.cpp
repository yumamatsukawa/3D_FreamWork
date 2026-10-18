#include "ModelDisplayController.h"
#include "../../../Engine/Render3D/ModelRenderer.h"
#include "../../../Engine/Animation/ModelAnimationComponent.h"

void ModelDisplayController::Update(float dt) {
    // アニメーション付き: 3秒ごとに、SpinとBobを滑らかに切り替える
    if (animation) {
        timer += dt;
        if (timer >= switchInterval) {
            timer = 0.f;
            spinning = !spinning;
            animation->CrossFade(spinning ? "Spin" : "Bob", fadeSeconds);
        }
        return;
    }

    // アニメーション無し: Topパーツだけ回す(無ければ何もしない。親のBaseパーツは動かない)
    Transform* top = renderer->GetNodeTransform("Top");
    if (top) top->rotate.y += spinSpeed * dt;
}