#pragma once
#include "../Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class RigidbodyComponent;

// 動く平面(2D=XY、3Dの地面=XZ)
enum class MovePlane { XY, XZ };

// 「目的地へ向かって動く」Component(ChaseComponent / AccelerateChaseComponent / HomingComponent)の親クラス。
// 単体ではAddComponentしない(コンストラクタはprotected)。
//
// 決まり:
// - 目的地は、SetTargetの相手がいればその位置、いなければSetTargetPositionの位置。どちらも無ければ止まる(速度0)
// - 相手が非アクティブ(SetActive(false)・Destroy済み)の間は、追わずに止まる
//   ※相手のGameObjectがDestroyされて本当に消えた後は、ポインタが消えた物を指したままになり、見分けられない。
//     相手を消す時は、先に追いかけている側でSetTarget(nullptr)すること(呼ぶ側の責任)
// - 距離・向きは動く平面の上だけで考える(XYならZの差、XZならYの差は無視する)
// - 動かし方: DynamicのRigidbodyComponentがあればSetVelocity。KinematicかRigidbodyが無ければtransform.positionを
//   直接動かす。Staticなら動かさない(出力ウィンドウに1回だけ知らせる)
// - XZの時は、Y方向の速度(重力で落ちる速さ)をそのまま残す。XYの時はZ方向の速度を0にする
// - ObjectPoolで再利用された時(SetActive(true))に、速度・進行方向などの状態をリセットする(相手・速さなどの設定は残る)
class MoveComponent : public Component {
public:
    void SetTarget(GameObject* newTarget) { target = newTarget; }                 // 追いかける相手(nullptrで外す)
    void SetTargetPosition(DirectX::XMFLOAT3 position);                          // 決まった場所へ向かう(相手がいない時に使う)
    void SetSpeed(float newSpeed) { speed = (newSpeed > 0.f) ? newSpeed : 0.f; } // 速さ(1秒に進む距離)
    void SetStopDistance(float distance) { stopDistance = (distance > 0.f) ? distance : 0.f; }   // これより近づいたら止まる
    void SetPlane(MovePlane newPlane) { plane = newPlane; }                      // 動く平面(既定はXY)
    bool HasArrived() const;                                                     // 着いたか(目的地がStopDistanceより近いか)

    GameObject* GetTarget() const { return target; }
    float GetSpeed() const { return speed; }

protected:
    GameObject* target = nullptr;            // 追いかける相手
    DirectX::XMFLOAT3 targetPosition{};      // 向かう場所(相手がいない時に使う)
    bool hasTargetPosition = false;          // SetTargetPositionされたか
    float speed = 100.0f;                    // 速さ
    float stopDistance = 1.0f;               // これより近づいたら止まる
    MovePlane plane = MovePlane::XY;         // 動く平面
    bool warnedStatic = false;               // 「Staticなので動かせません」を出力したか

    MoveComponent() = default;

    // 自分から目的地までのずれ(平面の外の成分は0)。目的地が無い・相手が非アクティブならfalse
    bool GetOffsetToDestination(DirectX::XMFLOAT3& offset) const;
    // ずれがStopDistanceより短いか
    bool IsWithinStopDistance(DirectX::XMFLOAT3 offset) const;
    // 平面の外の成分を0にする(XYならz、XZならy)
    DirectX::XMFLOAT3 ToPlane(DirectX::XMFLOAT3 v) const;
    // DynamicのRigidbodyComponent(無い・Dynamicでなければnullptr)
    RigidbodyComponent* GetDynamicRigidbody() const;
    // 速度を反映する(平面の外の成分は使わない)。dtはtransformを直接動かす時に使う
    void ApplyVelocity(DirectX::XMFLOAT3 velocity, float dt);
};
