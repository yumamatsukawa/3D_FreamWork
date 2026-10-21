#include "FloatingBall.h"
#include "../Common/DemoBody.h"
#include "../../Components/Render3D/BuoyancyController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateFloatingBall(Scene& scene, WaterRenderer* water, DirectX::XMFLOAT3 position, float diameter) {
    // 見た目(赤い球)と、物理の剛体(Dynamic。重力で落ちる)
    DemoBodyDesc desc;
    desc.shape = DemoShape::Round;
    desc.type = BodyType::Dynamic;
    desc.position = position;
    desc.size = diameter;
    desc.color = { 0.9f, 0.2f, 0.2f, 1.0f };
    desc.name = "FloatingBall";
    GameObject* ball = CreateDemoBody3D(scene, desc);

    // 動き: 沈んだ深さに応じた浮力で浮く
    ball->AddComponent<BuoyancyController>(water, diameter * 0.5f, position);
    return ball;
}
