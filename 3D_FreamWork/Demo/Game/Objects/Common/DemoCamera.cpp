#include "DemoCamera.h"
#include "../../Components/Common/DemoCameraController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateDemoCamera(Scene& scene, DirectX::XMFLOAT3 target, float yawDegrees, float pitchDegrees, float distance) {
    // 見る点に置いた、見た目の無いオブジェクト(カメラはこの周りを回る)
    GameObject* camera = scene.CreateObject("DemoCamera");
    camera->transform.position = target;
    camera->AddComponent<DemoCameraController>(DirectX::XMConvertToRadians(yawDegrees), DirectX::XMConvertToRadians(pitchDegrees), distance, 0.f);
    return camera;
}
