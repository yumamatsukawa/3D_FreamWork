#include "CameraController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/Image.h"
#include "../../../Engine/Graphics.h"
#include <cmath>

// マウスがゲーム画面(ウィンドウの中)にあるか。GetMousePositionは画面中央が原点で、画面の解像度の単位
bool IsMouseInScreen() {
    XMFLOAT2 mousePos = Input::GetMousePosition();
    return fabsf(mousePos.x) <= Graphics::width / 2.f && fabsf(mousePos.y) <= Graphics::height / 2.f;
}

void CameraController::Init() {
    // 有効な間は、Image::BeginFrame()による2Dカメラへの自動追従を止める
    Image::SetCamera3DAutoSync(false);

    // 始めはカーソルを固定した状態にする
    Input::SetCursorLocked(true);
}

void CameraController::Uninit() {
    // 他のオブジェクトの2D/3Dカメラ同期に影響しないよう、元に戻す
    Image::SetCamera3DAutoSync(true);

    // カーソルの固定を解除する(タイトル画面などでカーソルが消えたままにならないように)
    Input::SetCursorLocked(false);
}

bool CameraController::IsCursorLocked() const {
    return Input::IsCursorLocked();
}

void CameraController::UpdateCursorLock() {
    lockedThisFrame = false;

    // ESCで解除、ゲーム画面を左クリックで固定
    // (別のウィンドウを選ぶと、固定はInput側で自動的に解除される)
    if (Input::IsCursorLocked()) {
        if (Input::GetKeyDown(KEY_ESCAPE)) Input::SetCursorLocked(false);
    }
    else if (Input::GetKeyDown(MOUSE_LEFT) && IsMouseInScreen()) {
        Input::SetCursorLocked(true);
        lockedThisFrame = true;
    }

    // 視点回転: 固定中だけ、マウスの移動量で回す(固定したフレームは使わない)
    if (!Input::IsCursorLocked() || lockedThisFrame) return;
    XMFLOAT2 delta = Input::GetMouseDelta();
    yaw += delta.x * mouseSensitivity;
    pitch -= delta.y * mouseSensitivity;
    if (pitch < minPitch) pitch = minPitch;
    if (pitch > maxPitch) pitch = maxPitch;
}

XMFLOAT3 CameraController::ComputeForward() const {
    float cp = cosf(pitch);
    return { cp * sinf(yaw), sinf(pitch), cp * cosf(yaw) };
}

void CameraController::Update(float dt) {
    if (Input::GetKeyDown(KEY_TAB)) {
        mode = (mode == Mode::ThirdPerson) ? Mode::FirstPerson : Mode::ThirdPerson;
    }

    // カーソルの固定と、マウスの移動量での視点回転
    UpdateCursorLock();

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
