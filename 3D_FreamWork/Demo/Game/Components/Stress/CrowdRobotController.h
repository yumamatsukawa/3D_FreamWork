#pragma once
#include "../../../../Engine/Core/Component.h"

class RigidbodyComponent;

// 大群の中のロボット1体の動き(負荷テスト用)。一定時間ごとにランダムな向きと速さを決めて、床の上を歩き回る。
// 床の端に来たら中心の方へ向き直す。物理あり(剛体が付いている)の時は SetVelocity で動かし、物理なしの時は Transform を直接動かす
class CrowdRobotController : public Component {
public:
    static constexpr float arenaHalf = 1500.f;       // 歩き回る範囲の広さ(中心から端まで)

private:
    static constexpr float minSpeed = 60.f;          // 歩く速さの下限
    static constexpr float maxSpeed = 200.f;         // 歩く速さの上限
    static constexpr float minMoveTime = 1.f;        // 同じ向きで歩く時間の下限(秒)
    static constexpr float maxMoveTime = 3.f;        // 同じ向きで歩く時間の上限(秒)
    static constexpr float turnSpeed = 360.f;        // 見た目が向きを変える速さ(度/秒)

    RigidbodyComponent* body = nullptr;              // 剛体(物理なしの時は nullptr)
    float heading = 0.f;                             // 進む向き(度。0で+Z、増えると+X側)
    float faceYaw = 0.f;                             // 見た目の向き(度。heading へなめらかに回る)
    float speed = 0.f;                               // 歩く速さ
    float timer = 0.f;                               // 次に向きを決め直すまでの時間(秒)

    void ChooseMove();                               // ランダムな向き・速さ・歩く時間を決める

public:
    void Init() override;
    void Update(float dt) override;
};
