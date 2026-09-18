#pragma once
#include <DirectXMath.h>

using namespace DirectX;

// シンプルな平行光源(太陽光のように、全体に同じ方向から当たる光)+環境光。
// 3Dメッシュのライティング(陰影)に使う。
class Light {
public:
    XMFLOAT3 direction{ 0.0f, -1.0f, 0.5f }; // 光が進んでいく方向(正規化されていなくてもよい)
    XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };      // 光の色・強さ
    XMFLOAT3 ambient{ 0.25f, 0.25f, 0.25f }; // 環境光(光が当たらない面の最低限の明るさ)
};
