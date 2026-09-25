#include "CameraController.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/Input.h"
#include "../../Engine/Image.h"

void CameraController::Init() {
    // 有効な間は、Image::BeginFrame()による2Dカメラへの自動追従を止める
    Image::SetCamera3DAutoSync(false);
}

void CameraController::Uninit() {
    // 他のオブジェクトの2D/3Dカメラ同期に影響しないよう、元に戻す
    Image::SetCamera3DAutoSync(true);
}

XMFLOAT3 CameraController::ComputeForward() const {
    float cp = cosf(pitch);
    return { cp * sinf(yaw), sinf(pitch), cp * cosf(yaw) };
}

void CameraController::Update(float dt) {
    if (Input::GetKeyDown(KEY_TAB)) {
        mode = (mode == Mode::ThirdPerson) ? Mode::FirstPerson : Mode::ThirdPerson;
    }

    // 右クリックを押している間だけ、マウスの移動量で視点を回す
    XMFLOAT2 mousePos = Input::GetMousePosition();
    if (Input::GetKeyPress(MOUSE_RIGHT)) {
        if (hasLastMousePos) {
            float dx = mousePos.x - lastMousePos.x;
            float dy = mousePos.y - lastMousePos.y;
            yaw += dx * mouseSensitivity;
            pitch -= dy * mouseSensitivity;
            if (pitch < minPitch) pitch = minPitch;
            if (pitch > maxPitch) pitch = maxPitch;
        }
        hasLastMousePos = true;
    }
    else {
        hasLastMousePos = false;
    }
    lastMousePos = mousePos;

    XMFLOAT3 forward = ComputeForward();

    // Ownerのワールド座標を注視点にする(2D・3DともY+が上方向で統一されているので変換不要)
    Transform worldTransform = GetOwner()->transform.GetWorldTransform();
    XMFLOAT3 focusPoint = {
        worldTransform.position.x,
        worldTransform.position.y + eyeHeight,
        worldTransform.position.z
    };

    Camera& camera3D = Image::GetCamera3D();

    if (mode == Mode::ThirdPerson) {
        camera3D.position = {
            focusPoint.x - forward.x * distance,
            focusPoint.y - forward.y * distance + thirdPersonHeight,
            focusPoint.z - forward.z * distance
        };
        camera3D.target = focusPoint;
    }
    else {
        camera3D.position = focusPoint;
        camera3D.target = {
            focusPoint.x + forward.x,
            focusPoint.y + forward.y,
            focusPoint.z + forward.z
        };
    }
}
