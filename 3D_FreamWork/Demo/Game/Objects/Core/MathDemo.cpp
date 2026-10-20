#include "MathDemo.h"
#include "../../Components/Core/MathDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateMathDemo(Scene& scene, DemoMode mode) {
    // 関数の選択・値の更新・点と線の描画をする役
    GameObject* demo = scene.CreateObject("MathDemo");
    demo->AddComponent<MathDemoController>(mode);
    return demo;
}
