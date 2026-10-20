#pragma once
#include <DirectXMath.h>

// 3Dデモで「マウスの下の地面」を求める。マウスの位置から奥へ伸ばした線(Image::ScreenPointToRay3D)が、
// 高さ groundY の水平な面に当たる場所を point に入れる。当たらなければ(空を向いている時など) false
bool GetMouseGroundPoint(float groundY, DirectX::XMFLOAT3& point);
