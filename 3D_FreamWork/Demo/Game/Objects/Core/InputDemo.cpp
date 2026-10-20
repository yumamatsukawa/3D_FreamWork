#include "InputDemo.h"
#include "../../Components/Core/InputDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateInputDemo(Scene& scene) {
    // 入力の状態を毎フレーム読んで画面に出す役
    GameObject* demo = scene.CreateObject("InputDemo");
    demo->AddComponent<InputDemoController>();
    return demo;
}
