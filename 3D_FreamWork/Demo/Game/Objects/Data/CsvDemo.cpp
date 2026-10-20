#include "CsvDemo.h"
#include "../../Components/Data/CsvDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateCsvDemo(Scene& scene) {
    // 表の読み込み・ホットリロード・表示は、操作役がすべて行う
    GameObject* demo = scene.CreateObject("CsvDemo");
    demo->AddComponent<CsvDemoController>();
    return demo;
}