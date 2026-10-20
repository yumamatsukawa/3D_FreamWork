#pragma once
#include "../../../../Engine/Core/Component.h"

// デモ用の3Dカメラ。付けたオブジェクトの位置(+focusHeight)を見ながら、周りを回る。
// 右ボタンを押しながらマウスを動かすと向きが変わり、Q/Eで近づく・離れる(カーソルは固定しないので、ボタンも押せる)。
// Image::GetCamera3D()を直接操作するので、有効な間は2Dカメラへの自動追従を止めておく(Init/Uninitで自動的に切り替える)
class DemoCameraController : public Component {
private:
    float yaw;                 // 水平方向の向き(ラジアン。0で+Z方向を向き、増えるほど+X方向(右)へ回る)
    float pitch;               // 上下の向き(ラジアン。マイナスで見下ろす)
    float distance;            // 見ている点からカメラまでの距離
    float focusHeight;         // 付けたオブジェクトの位置から、見る点までの高さ

    static constexpr float mouseSensitivity = 0.004f;   // マウス1ピクセルあたりの回転量(ラジアン)
    static constexpr float zoomSpeed = 600.f;           // Q/Eを押している間に、距離が変わる速さ(1秒あたり)
    static constexpr float minDistance = 150.f;         // 近づける限界
    static constexpr float maxDistance = 3000.f;        // 離れられる限界
    static constexpr float minPitch = -1.3f;            // 約-75度
    static constexpr float maxPitch = 0.6f;             // 約 35度

public:
    DemoCameraController(float startYaw, float startPitch, float startDistance, float height)
        : yaw(startYaw), pitch(startPitch), distance(startDistance), focusHeight(height) {}

    float GetYaw() const { return yaw; }   // 水平方向の向き(歩く向きの基準にする)

    void Init() override;
    void Update(float dt) override;
    void Uninit() override;
};
