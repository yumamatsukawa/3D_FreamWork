#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// 動く壁(ドア)の動き。閉じた位置と開いた位置の間を、一定の速さで動く(Kinematic の剛体の Transform を動かす。物理の壁なので、経路探索の格子が焼き直しで拾う)。
// 開け閉めは SetOpen で指示する。動いている間は IsMoving が true
class DoorController : public Component {
private:
    DirectX::XMFLOAT3 closedPosition{};     // 閉じた位置
    DirectX::XMFLOAT3 openPosition{};       // 開いた位置
    float speed = 150.f;                    // 動く速さ(1秒に進む距離)
    bool open = false;                      // 開ける指示が出ているか
    bool moving = false;                    // 動いている最中か

public:
    // 位置と速さを、CreateMovingWallDemo()から渡してもらう(Init の後。今の位置を閉じた位置として覚える)
    void Setup(DirectX::XMFLOAT3 closed, DirectX::XMFLOAT3 opened, float moveSpeed);

    void SetOpen(bool value) { open = value; }       // 開ける(true)/閉める(false)
    bool IsOpen() const { return open; }
    bool IsMoving() const { return moving; }
    DirectX::XMFLOAT3 GetClosedPosition() const { return closedPosition; }   // 閉じた位置(ドアの通り道の真ん中)

    void Update(float dt) override;
};
