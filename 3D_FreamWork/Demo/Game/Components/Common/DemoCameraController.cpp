#include "DemoCameraController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include <cmath>

void DemoCameraController::Init() {
    // 有効な間は、Image::BeginFrame()による2Dカメラへの自動追従を止める
    Image::SetCamera3DAutoSync(false);
    Image::GetCamera3D().fovY = DirectX::XMConvertToRadians(60.f);
}

void DemoCameraController::Uninit() {
    // 他のシーンのカメラに影響しないよう、元に戻す
    Image::SetCamera3DAutoSync(true);
}

void DemoCameraController::Update(float dt) {
    // 向きの変更: 右ボタンを押している間だけ、マウスの移動量で回す
    if (Input::GetKeyPress(MOUSE_RIGHT)) {
        DirectX::XMFLOAT2 delta = Input::GetMouseDelta();
        yaw += delta.x * mouseSensitivity;
        pitch = Math::Clamp(pitch - delta.y * mouseSensitivity, minPitch, maxPitch);
    }

    // ズーム: Qで近づく、Eで離れる
    if (Input::GetKeyPress(KEY_Q)) distance -= zoomSpeed * dt;
    if (Input::GetKeyPress(KEY_E)) distance += zoomSpeed * dt;
    distance = Math::Clamp(distance, minDistance, maxDistance);

    // 見る点(付けたオブジェクトの位置の少し上)から、向きの反対へ distance 離れた所にカメラを置く
    DirectX::XMFLOAT3 ownerPos = GetOwner()->transform.GetWorldTransform().position;
    DirectX::XMFLOAT3 focus = { ownerPos.x, ownerPos.y + focusHeight, ownerPos.z };
    float cp = cosf(pitch);
    DirectX::XMFLOAT3 forward = { cp * sinf(yaw), sinf(pitch), cp * cosf(yaw) };

    Camera& camera = Image::GetCamera3D();
    camera.position = { focus.x - forward.x * distance, focus.y - forward.y * distance, focus.z - forward.z * distance };
    camera.target = focus;
}
