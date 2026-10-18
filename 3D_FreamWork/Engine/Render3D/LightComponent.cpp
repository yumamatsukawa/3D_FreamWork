#include "LightComponent.h"
#include "Lighting.h"
#include "../Core/GameObject.h"
#include "../Core/MathUtil.h"
#include <cmath>

void LightComponent::SetSpotAngle(float innerDegrees, float outerDegrees) {
    // 0〜89度に収める(90度以上だと円すいの計算が成り立たない)。内側が外側より大きい時は外側に合わせる
    if (outerDegrees < 0.f) outerDegrees = 0.f;
    if (outerDegrees > 89.f) outerDegrees = 89.f;
    if (innerDegrees < 0.f) innerDegrees = 0.f;
    if (innerDegrees > outerDegrees) innerDegrees = outerDegrees;
    innerAngle = innerDegrees;
    outerAngle = outerDegrees;
}

XMFLOAT3 LightComponent::GetWorldPosition() const {
    // ワールド行列の位置の部分(親がいれば親の動きも入る)
    XMFLOAT3 position;
    XMStoreFloat3(&position, GetOwner()->transform.GetWorldMatrix().r[3]);
    return position;
}

XMFLOAT3 LightComponent::GetWorldDirection() const {
    XMFLOAT3 result = direction;
    if (result.x * result.x + result.y * result.y + result.z * result.z < 1e-8f) {
        result = Math::Forward3D(GetOwner()->transform);   // 向きの指定が無ければ、オーナーの水平の正面
    }
    XMVECTOR v = XMVector3Normalize(XMLoadFloat3(&result));
    XMStoreFloat3(&result, v);
    return result;
}

LightComponent::~LightComponent() {
    Lighting::Unregister(this);   // Uninitが呼ばれなかった時も、古いポインタが残らないように
}

void LightComponent::Init() {
    Lighting::Register(this);
}

void LightComponent::Uninit() {
    Lighting::Unregister(this);
}
