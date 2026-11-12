#include "CrowdRobotController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <cmath>

void CrowdRobotController::ChooseMove() {
    // ランダムな向き・速さ・歩く時間
    heading = Math::RandomRange(0.f, 360.f);
    speed = Math::RandomRange(minSpeed, maxSpeed);
    timer = Math::RandomRange(minMoveTime, maxMoveTime);
}

void CrowdRobotController::Init() {
    // 剛体が付いていれば物理あり(CreateCrowdRobot が先に付けておく)
    body = GetOwner()->GetComponent<RigidbodyComponent>();
    ChooseMove();
    faceYaw = heading;
}

void CrowdRobotController::Update(float dt) {
    Transform& transform = GetOwner()->transform;

    // 向きの決め直し
    timer -= dt;
    if (timer <= 0.f) ChooseMove();

    // 床の端: 外へ出ていたら中心の方へ向き直す(しばらくそのまま歩く)
    if (fabsf(transform.position.x) > arenaHalf || fabsf(transform.position.z) > arenaHalf) {
        heading = Math::AngleToTargetY(transform.position, { 0.f, transform.position.y, 0.f });
        if (timer < minMoveTime) timer = minMoveTime;
        // 物理なしは壁が無いので、位置も端に戻す
        if (!body) {
            transform.position.x = Math::Clamp(transform.position.x, -arenaHalf, arenaHalf);
            transform.position.z = Math::Clamp(transform.position.z, -arenaHalf, arenaHalf);
        }
    }

    // 移動処理: 物理ありは速度で、物理なしは位置を直接動かす
    const float radian = DirectX::XMConvertToRadians(heading);
    const DirectX::XMFLOAT3 velocity = { sinf(radian) * speed, 0.f, cosf(radian) * speed };
    if (body) {
        // 上下は重力に任せる(今の落下の速さを残す)
        body->SetVelocity({ velocity.x, body->GetVelocity().y, velocity.z });
    } else {
        transform.position.x += velocity.x * dt;
        transform.position.z += velocity.z * dt;
    }

    // 見た目の向き: 進む向きへなめらかに回る(ロボットの顔は-Z側なので、180度足す)
    faceYaw = Math::MoveTowardsAngle(faceYaw, heading, turnSpeed * dt);
    transform.rotate.y = faceYaw + 180.f;
}
