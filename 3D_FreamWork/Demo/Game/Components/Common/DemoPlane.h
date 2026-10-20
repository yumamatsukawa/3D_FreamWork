#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Behavior/MoveComponent.h"
#include <DirectXMath.h>

// 動きとAI(Behavior)のデモが共有する「動く平面」の道具。
// 2Dは XY の面(z=0)、3Dは地面の上の XZ の面で動く。平面の点は (横, 縦) の XMFLOAT2 で表す(2Dは x,y、3Dは x,z)

constexpr float demoGroundTop = -50.f;   // 3Dの地面の上面の高さ(DemoGround と同じ)

// 動く平面(2D=XY、3D=XZ)
MovePlane GetDemoPlane(DemoMode mode);

// 平面の点 → ワールドの位置。lift: 3Dで地面の上面からの高さ(球なら半径)。2Dは無視して z=0
DirectX::XMFLOAT3 ConvertPlaneToWorld(DemoMode mode, DirectX::XMFLOAT2 point, float lift);

// ワールドの位置 → 平面の点(2Dは x,y、3Dは x,z)
DirectX::XMFLOAT2 ConvertWorldToPlane(DemoMode mode, const DirectX::XMFLOAT3& position);

// マウスの下の位置(2Dはマウスのワールドの位置、3Dはマウスの線が「地面の上面から lift の高さ」の水平な面に当たる場所)。3Dで空を向いている時は false
bool GetDemoMousePoint(DemoMode mode, float lift, DirectX::XMFLOAT3& point);

// 標準の競技場(壁で囲んだ範囲)。Behavior のデモは、2D版も3D版もこの範囲を使う(平面の点で、内側の min〜max)
struct DemoField {
    DirectX::XMFLOAT2 min;
    DirectX::XMFLOAT2 max;
};
DemoField GetDemoField(DemoMode mode);

// 点を競技場の中(縁から margin 内側)に収める
DirectX::XMFLOAT2 ClampToDemoField(const DemoField& field, DirectX::XMFLOAT2 point, float margin);
