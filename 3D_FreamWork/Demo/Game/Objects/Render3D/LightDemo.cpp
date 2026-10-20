#include "LightDemo.h"
#include "../../Components/Render3D/LightDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateLightDemo(Scene& scene, GameObject* point, GameObject* spot) {
    GameObject* demo = scene.CreateObject("LightDemo");
    // 操作と表示、と、切り替える光
    demo->AddComponent<LightDemoController>()->Setup(point, spot);
    return demo;
}
