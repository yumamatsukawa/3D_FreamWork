#include "SaveDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/Data/SaveDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateSaveDemo(Scene& scene) {
    // 操作役(値の読み書き・保存・読込・表示)。先に作って、ボタンから呼べるようにする
    GameObject* demo = scene.CreateObject("SaveDemo");
    SaveDemoController* controller = demo->AddComponent<SaveDemoController>();

    // ボタン(キーと同じ。左の列=値を変える、真ん中=ファイル、右=スロット)
    const DirectX::XMFLOAT2 size = { 270.f, 52.f };
    CreateDemoButton(scene, L"Z Count +1", { -400.f, -10.f }, size, 24.f, [controller] { controller->AddCount(); });
    CreateDemoButton(scene, L"X Ratio +0.1", { -400.f, -75.f }, size, 24.f, [controller] { controller->AddRatio(); });
    CreateDemoButton(scene, L"C Next Name", { -400.f, -140.f }, size, 24.f, [controller] { controller->NextName(); });
    CreateDemoButton(scene, L"S Save", { -100.f, -10.f }, size, 24.f, [controller] { controller->SaveNow(); });
    CreateDemoButton(scene, L"L Load", { -100.f, -75.f }, size, 24.f, [controller] { controller->LoadNow(); });
    CreateDemoButton(scene, L"D Delete Keys", { -100.f, -140.f }, size, 24.f, [controller] { controller->DeleteKeys(); });
    CreateDemoButton(scene, L"1 Slot 1", { 200.f, -10.f }, size, 24.f, [controller] { controller->SelectSlot(1); });
    CreateDemoButton(scene, L"2 Slot 2", { 200.f, -75.f }, size, 24.f, [controller] { controller->SelectSlot(2); });
    CreateDemoButton(scene, L"3 Slot 3", { 200.f, -140.f }, size, 24.f, [controller] { controller->SelectSlot(3); });
    return demo;
}