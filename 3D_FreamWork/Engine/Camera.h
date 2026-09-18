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

    // ★ 深度バッファの精度はnearZに強く依存し、nearZ/farZの比が大きいほど
    //   遠く(スカイボックスなど)の精度が失われてZ-fighting(継ぎ目のちらつき・隙間)の
    //   原因になる。このゲームのスケール(オブジェクトは数百〜数千単位)では
    //   nearZ=0.1のような極端に小さい値は不要なので、5に上げて精度を確保している
    float nearZ = 5.0f;      // これより手前は描画しない
    float farZ = 10000.f;    // これより奥は描画しない

    // 2D描画で「奥行きによる遠近感」を出すための基準距離。
    // オブジェクトがこの距離にいる時、等倍(100%)の大きさで表示される。
    // 既定値はposition.zの初期値(-500)に合わせてあるので、
    // Z=0のオブジェクトは今まで通りの見た目のまま
    float focalLength = 500.f;

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
