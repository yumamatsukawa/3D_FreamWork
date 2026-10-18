#pragma once
#include "MoveComponent.h"
#include <DirectXMath.h>

// 目的地(相手・場所)へ、加速しながら向かうComponent。勢いがあるので行き過ぎて戻ってくる。
// StopDistanceより近い間は加速をやめる(勢いのまま進み、空気抵抗で遅くなる)。決まりはMoveComponentのコメントを参照。
// - DynamicのRigidbodyComponentがあれば、AddForce(ForceMode::Acceleration)で加速する(ぶつかった時の跳ね返りも残る)
//   ※1フレーム分の加速だけ、最高速度を少し超えることがある(次のフレームで最高速度に戻す)
// - KinematicかRigidbodyが無ければ、自分で速度を持って動かす
// - 最高速度はSetSpeedとSetMaxSpeedのどちらで決めてもよい(同じもの)
//
// 使い方:
//   auto* chase = obj->AddComponent<AccelerateChaseComponent>();
//   chase->SetTarget(player);
//   chase->SetAcceleration(300.f);
//   chase->SetMaxSpeed(200.f);
//   chase->SetDrag(1.0f);
class AccelerateChaseComponent : public MoveComponent {
private:
    float acceleration = 200.0f;              // 加速の強さ(1秒に速さがいくつ増えるか)
    float drag = 0.0f;                        // 空気抵抗(0で減速しない。大きいほどすぐ減速する)
    DirectX::XMFLOAT3 velocity{};             // 今の速度(Kinematic・Rigidbody無しの時に使う)

public:
    void SetAcceleration(float value) { acceleration = (value > 0.f) ? value : 0.f; }   // 加速の強さ
    void SetMaxSpeed(float maxSpeed) { SetSpeed(maxSpeed); }                            // 最高速度(SetSpeedと同じ)
    void SetDrag(float value) { drag = (value > 0.f) ? value : 0.f; }                   // 空気抵抗(負の値は0にする)

    void Update(float dt) override;
    void OnActiveChanged(bool active) override;
};
