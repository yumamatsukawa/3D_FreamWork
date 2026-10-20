#include "DemoPlane.h"
#include "DemoMouseGround.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"

MovePlane GetDemoPlane(DemoMode mode) {
    return (mode == DemoMode::Mode2D) ? MovePlane::XY : MovePlane::XZ;
}

DirectX::XMFLOAT3 ConvertPlaneToWorld(DemoMode mode, DirectX::XMFLOAT2 point, float lift) {
    if (mode == DemoMode::Mode2D) return { point.x, point.y, 0.f };
    return { point.x, demoGroundTop + lift, point.y };
}

DirectX::XMFLOAT2 ConvertWorldToPlane(DemoMode mode, const DirectX::XMFLOAT3& position) {
    if (mode == DemoMode::Mode2D) return { position.x, position.y };
    return { position.x, position.z };
}

bool GetDemoMousePoint(DemoMode mode, float lift, DirectX::XMFLOAT3& point) {
    if (mode == DemoMode::Mode2D) {
        const DirectX::XMFLOAT2 mouse = Input::GetMouseWorldPosition();   // カメラ位置を考慮済み(Y+=上)
        point = { mouse.x, mouse.y, 0.f };
        return true;
    }
    return GetMouseGroundPoint(demoGroundTop + lift, point);
}

DemoField GetDemoField(DemoMode mode) {
    // 2D: 画面の左上の板・左下右下のボタンと重ならない範囲。3D: 見下ろすカメラ(CreateDemoStage3D)で画面に収まる範囲
    if (mode == DemoMode::Mode2D) return { { -600.f, -250.f }, { 600.f, 50.f } };
    return { { -750.f, -300.f }, { 750.f, 350.f } };
}

DirectX::XMFLOAT2 ClampToDemoField(const DemoField& field, DirectX::XMFLOAT2 point, float margin) {
    return { Math::Clamp(point.x, field.min.x + margin, field.max.x - margin),
             Math::Clamp(point.y, field.min.y + margin, field.max.y - margin) };
}
