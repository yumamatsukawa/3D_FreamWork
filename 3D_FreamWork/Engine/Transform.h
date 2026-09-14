#pragma once
#include <DirectXMath.h>

using namespace DirectX;

static struct Transform {
    XMFLOAT3 position{}; // 位置
    XMFLOAT3 scale{};    // 大きさ
    XMFLOAT3 rotate{};   // 度数
};