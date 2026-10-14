#pragma once
#include "MoveComponent.h"

// 曲がれる角度に上限がある追尾(ミサイル・車・魚向け)。今の進行方向を持ち、目的地の方へ
// 1秒に最大turnSpeed度だけ曲がりながら、speedの速さで進む。決まりはMoveComponentのコメントを参照。
// - 進行方向の角度は rotate と同じ向き(XYはrotate.z = Math::Forward2D、XZはrotate.y = Math::Forward3D)
// - 最初の進行方向は、出した時(最初のUpdate・ObjectPoolで再利用された後の最初のUpdate)のrotateの向き。
//   出す時にrotateを決めておけば、その向きへ飛び出してから曲がっていく
// - SetFaceMovement(true)なら、見た目の向き(rotate)も進行方向に合わせる。
//   Dynamicの剛体で使う時は SetFreezeRotation(true) にしておく(しないとPhysXの回転で上書きされる)
//
// 使い方:
//   auto* homing = missile->AddComponent<HomingComponent>();
//   homing->SetTarget(enemy);
//   homing->SetSpeed(300.f);
//   homing->SetTurnSpeed(120.f);     // 1秒に120度まで曲がれる
//   homing->SetFaceMovement(true);
class HomingComponent : public MoveComponent {
private:
    float turnSpeed = 180.0f;          // 1秒に何度まで曲がれるか
    bool faceMovement = false;         // 見た目の向き(rotate)も進行方向に合わせるか
    float heading = 0.0f;              // 今の進行方向(度。XYはrotate.z、XZはrotate.yと同じ向き)
    bool headingInitialized = false;   // headingをrotateから読んだか(出した後の最初のUpdateで読む)

public:
    void SetTurnSpeed(float degreesPerSecond) { turnSpeed = (degreesPerSecond > 0.f) ? degreesPerSecond : 0.f; }   // 1秒に何度まで曲がれるか(負の値は0にする)
    void SetFaceMovement(bool enabled) { faceMovement = enabled; }   // 見た目の向きも進行方向に合わせる

    void Update(float dt) override;
    void OnActiveChanged(bool active) override;
};
