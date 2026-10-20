#include "DemoStage3D.h"
#include "DemoSkybox.h"
#include "DemoGround.h"
#include "DemoCamera.h"

GameObject* CreateDemoStage3D(Scene& scene, DirectX::XMFLOAT3 target, float yawDegrees, float pitchDegrees, float distance) {
    // 背景(空)→ 地面 → カメラの順に置く
    CreateDemoSkybox(scene);
    CreateDemoGround(scene);
    return CreateDemoCamera(scene, target, yawDegrees, pitchDegrees, distance);
}
