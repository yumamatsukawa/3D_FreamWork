#include "RectDemo.h"
#include "../../Components/UI/RectDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateRectDemo(Scene& scene) {
    // 四角を描き、ドラッグを受け付ける役
    GameObject* demo = scene.CreateObject("RectDemo");
    demo->AddComponent<RectDemoController>();
    return demo;
}
