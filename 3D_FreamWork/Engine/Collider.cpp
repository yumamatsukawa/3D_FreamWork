#include "Collider.h"
#include "GameObject.h"
#include <cmath>

using namespace DirectX;

const std::string& Collider2D::GetTag() const {
    return owner->GetTag();
}

bool IsPointInBox(XMFLOAT2 point, const Transform& transform) {
    // 回転を考慮:点をオブジェクトのローカル座標系に変換して判定
    float dx = point.x - transform.position.x;
    float dy = point.y - transform.position.y;

    float rad = -transform.rotate.z * (XM_PI / 180.f);  // 逆回転
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    float localX = dx * cosA - dy * sinA;
    float localY = dx * sinA + dy * cosA;

    float halfW = transform.scale.x / 2.f;
    float halfH = transform.scale.y / 2.f;

    return (localX >= -halfW && localX <= halfW &&
        localY >= -halfH && localY <= halfH);
}
