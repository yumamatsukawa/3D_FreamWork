#include "HighlightController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ModelRenderer.h"
#include "../../../Engine/Physics.h"

void HighlightController::Init() {
    // 同じオブジェクトに付いているモデルの見た目を探す(作る側で、このComponentより先に付けておく)
    renderer = GetOwner()->GetComponent<ModelRenderer>();
}

void HighlightController::Update(float dt) {
    if (!renderer) return;

    // 光る判定: 近くに"Player"タグの物がいれば、枠線を出す(離れたら消す)
    bool nearby = !Physics::OverlapSphere(GetOwner()->transform.position, radius, "Player").empty();
    renderer->SetOutline(color, nearby ? width : 0.0f);
}