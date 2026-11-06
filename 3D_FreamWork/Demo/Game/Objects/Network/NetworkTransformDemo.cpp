#include "NetworkTransformDemo.h"
#include "NetworkBody.h"
#include "../../Components/Network/NetworkMoverController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"

namespace {
    // シーンに置く物の NetworkId(1〜999。ホストとクライアントで同じ番号にする)
    constexpr int moverId = 10;
    constexpr int ballId = 11;   // 11, 12

    // 2D版(壁で囲んだ場所は NetworkArena)
    const DirectX::XMFLOAT3 moverCenter2D = { 0.f, -195.f, 0.f };
    constexpr float moverRadius2D = 95.f;

    // 3D版
    const DirectX::XMFLOAT3 moverCenter3D = { 0.f, 0.f, 0.f };
    constexpr float moverRadius3D = 250.f;
}

GameObject* CreateNetworkTransformDemo(Scene& scene, DemoMode mode) {
    const bool is3D = (mode == DemoMode::Mode3D);

    // 円を描いて動く箱(Kinematic。ホストが動かす)
    DemoBodyDesc mover;
    mover.shape = DemoShape::Block;
    mover.type = BodyType::Kinematic;
    mover.size = is3D ? 100.f : 70.f;
    mover.color = { 1.f, 0.6f, 0.2f, 1.f };
    mover.name = "NetworkMover";
    mover.position = is3D ? DirectX::XMFLOAT3{ moverCenter3D.x + moverRadius3D, moverCenter3D.y, moverCenter3D.z }
                          : DirectX::XMFLOAT3{ moverCenter2D.x + moverRadius2D, moverCenter2D.y, 0.f };
    mover.canRotate = true;
    GameObject* moverObject = CreateNetworkBody(scene, mode, moverId, mover);
    moverObject->AddComponent<NetworkMoverController>(is3D, is3D ? moverCenter3D : moverCenter2D, is3D ? moverRadius3D : moverRadius2D);

    // 押される球・丸(Dynamic。ホストだけ物理で動く。クライアントのコピーは Kinematic になる)
    for (int i = 0; i < 2; i++) {
        const float side = (i == 0) ? 1.f : -1.f;
        DemoBodyDesc ball;
        ball.shape = DemoShape::Round;
        ball.type = BodyType::Dynamic;
        ball.size = is3D ? 90.f : 60.f;
        ball.color = { 0.4f, 0.6f, 1.f, 1.f };
        ball.cell = 6;
        ball.name = "NetworkBall";
        ball.canRotate = true;
        ball.position = is3D ? DirectX::XMFLOAT3{ side * 160.f, 250.f + 150.f * (float)i, 40.f * side }
                             : DirectX::XMFLOAT3{ side * 150.f, moverCenter2D.y, 0.f };
        GameObject* ballObject = CreateNetworkBody(scene, mode, ballId + i, ball);
        // 2Dは重力が無いので、押された後に止まるよう、空気抵抗を付ける(Dynamicの時だけ効く)
        if (!is3D) {
            if (RigidbodyComponent* rigidbody = ballObject->GetComponent<RigidbodyComponent>()) rigidbody->SetDrag(1.5f);
        }
    }

    return moverObject;
}