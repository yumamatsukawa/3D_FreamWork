#include "CrowdRobot.h"
#include "../../Components/Stress/CrowdRobotController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"

namespace {
    constexpr float groundTop = -50.f;                  // 地面の上面の高さ(DemoGroundと同じ)
    const wchar_t* robotPath = L"Assets/Models/robot.fbx";   // 置くFBX(32パーツの親子。ModelDemoと同じ)

    // ロボットの大きさ(ModelDemoと同じ。元は高さ約6なので28倍にして高さ約167にする)。
    // 足元は原点の約2.995(元の値)下にあるので、拡大した分だけ持ち上げて、足を地面に着ける
    constexpr float robotScale = 28.f;
    constexpr float robotBottom = 2.995f * robotScale;  // 拡大した後の、原点から足元までの距離
    constexpr float bodyRadius = 36.f;                  // 剛体の半径(ロボットの幅の半分くらい)
    constexpr float bodyHeight = robotBottom * 2.f;     // 剛体の高さ(原点が真ん中になる)

    // 距離で省く設定(LODのスイッチをオンにした時だけ効く)
    constexpr float drawDistance = 3500.f;              // これより遠いロボットは描かない
    constexpr float shadowDistance = 2800.f;            // これより遠いロボットは影を落とさない
    constexpr float outlineDistance = 2200.f;           // これより遠いロボットは枠線を描かない
}

GameObject* CreateCrowdRobot(Scene& scene, float x, float z, bool physics) {
    GameObject* robot = scene.CreateObject("CrowdRobot", "Robot");
    robot->transform.position = { x, groundTop + robotBottom, z };
    robot->transform.scale = { robotScale, robotScale, robotScale };

    // 見た目(読めない時は何も描かない)
    ModelRenderer* renderer = robot->AddComponent<ModelRenderer>(robotPath);
    renderer->GetMaterial().specularStrength = 0.3f;
    renderer->SetLod({ drawDistance, shadowDistance, outlineDistance });

    // 物理演算(PhysX): 物理ありの時だけ、カプセルの剛体を付ける(Controller より先に付ける)
    if (physics) {
        auto* body = robot->AddComponent<CapsuleRigidbodyComponent>(BodyType::Dynamic, bodyRadius, bodyHeight);
        body->SetFreezeRotation(true);
    }

    // 動き: ランダムに歩き回る
    robot->AddComponent<CrowdRobotController>();
    return robot;
}
