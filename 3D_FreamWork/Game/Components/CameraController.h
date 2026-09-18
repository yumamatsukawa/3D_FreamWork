#pragma once
#include "../../Engine/Component.h"
#include <DirectXMath.h>

using namespace DirectX;

// Ownerを追いかける3D専用のカメラ。TABキーで三人称/一人称を切り替えられる。
// 右クリックを押している間、マウスの移動量で視点を回転できる。
// Image::GetCamera3D()を直接操作するので、有効な間はImage::BeginFrame()による
// 2Dカメラへの自動追従を止めておく(Init/Uninitで自動的に切り替える)
class CameraController : public Component {
public:
    enum class Mode { ThirdPerson, FirstPerson };

private:
    Mode mode = Mode::ThirdPerson;

    float yaw = 0.0f;    // 水平方向の視点角度(ラジアン)
    float pitch = 0.0f;  // 上下方向の視点角度(ラジアン。まっすぐ前を向くと0)

    float distance = 300.0f;          // 三人称: 注視点からカメラまでの距離
    float thirdPersonHeight = 80.0f;  // 三人称: 注視点より上に足すオフセット
    float eyeHeight = 20.0f;          // 一人称: Owner位置からの目線の高さオフセット
    float mouseSensitivity = 0.005f;
    float minPitch = -1.3f; // 約-75度
    float maxPitch = 1.3f;  // 約 75度

    XMFLOAT2 lastMousePos{ 0.f, 0.f };
    bool hasLastMousePos = false;

    XMFLOAT3 ComputeForward() const;

public:
    void Init() override;
    void Update(float dt) override;
    void Uninit() override;

    void SetMode(Mode m) { mode = m; }
    Mode GetMode() const { return mode; }
};
