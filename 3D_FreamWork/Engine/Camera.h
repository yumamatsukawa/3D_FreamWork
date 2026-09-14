#pragma once
#include <DirectXMath.h>

using namespace DirectX;

// 「どこから」「どう見るか」を表すカメラ。
// 描画対象(Sprite/Meshなど)の行列計算から独立させることで、
// カメラを動かす・切り替えるだけで見た目を変えられるようにする。
class Camera {
public:
    XMFLOAT3 position{ 0.f, 0.f, -500.f }; // カメラ自身のワールド座標
    XMFLOAT3 target{ 0.f, 0.f, 0.f };      // 注視点(どこを見ているか)
    XMFLOAT3 up{ 0.f, 1.f, 0.f };          // 上方向の基準

    float fovY = XM_PIDIV4;  // 縦方向の視野角(ラジアン)。デフォルトは45度
    float nearZ = 0.1f;      // これより手前は描画しない
    float farZ = 10000.f;    // これより奥は描画しない

    // ワールド空間 → カメラ空間 への変換行列
    XMMATRIX GetViewMatrix() const {
        XMVECTOR eye = XMLoadFloat3(&position);
        XMVECTOR at = XMLoadFloat3(&target);
        XMVECTOR upVec = XMLoadFloat3(&up);
        return XMMatrixLookAtLH(eye, at, upVec);
    }

    // カメラ空間 → 透視投影(奥行きがあるほど小さく見える3D用)
    XMMATRIX GetProjectionMatrix(float aspectRatio) const {
        return XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearZ, farZ);
    }

    // カメラ空間 → 正射影(奥行きによる縮小がない2D/UI向け)
    static XMMATRIX GetOrthographicMatrix(float width, float height,
        float nearZ = 0.1f, float farZ = 10000.f) {
        return XMMatrixOrthographicLH(width, height, nearZ, farZ);
    }
};
