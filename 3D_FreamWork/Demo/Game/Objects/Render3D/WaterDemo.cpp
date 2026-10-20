#include "WaterDemo.h"
#include "../../Components/Render3D/WaterDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateWaterDemo(Scene& scene, WaterRenderer* water) {
    GameObject* demo = scene.CreateObject("WaterDemo");
    // 操作と表示、と、設定を変える水面
    demo->AddComponent<WaterDemoController>()->Setup(water);
    return demo;
}
