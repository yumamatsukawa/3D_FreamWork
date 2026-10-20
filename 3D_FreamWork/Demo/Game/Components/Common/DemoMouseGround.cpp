#include "DemoMouseGround.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Render/Image.h"

bool GetMouseGroundPoint(float groundY, DirectX::XMFLOAT3& point) {
    // マウスの位置から奥へ伸ばした線(Input::GetMousePositionはY+=下なので、Yの符号を反転する)
    const DirectX::XMFLOAT2 mouse = Input::GetMousePosition();
    DirectX::XMFLOAT3 origin, direction;
    Image::ScreenPointToRay3D({ mouse.x, -mouse.y }, origin, direction);

    // 地面に向かっていない(空を向いている)・地面が後ろにある時は当たらない
    if (direction.y > -0.0001f) return false;
    const float distance = (groundY - origin.y) / direction.y;
    if (distance < 0.f) return false;

    point = { origin.x + direction.x * distance, groundY, origin.z + direction.z * distance };
    return true;
}
