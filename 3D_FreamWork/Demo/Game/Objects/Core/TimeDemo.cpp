#include "TimeDemo.h"
#include "../../Components/Core/TimeDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"

GameObject* CreateTimeDemo(Scene& scene) {
    // 時間の倍率の影響を受けて回る物(dtで回る)
    GameObject* scaled = scene.CreateObject("ScaledSpinner");
    scaled->transform.position = { -400.f, -40.f, 0.f };
    scaled->transform.scale = { 100.f, 100.f, 1.f };
    scaled->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(1);

    // 倍率の影響を受けずに回る物(UnscaledDeltaTimeで回る)
    GameObject* unscaled = scene.CreateObject("UnscaledSpinner");
    unscaled->transform.position = { -110.f, -40.f, 0.f };
    unscaled->transform.scale = { 100.f, 100.f, 1.f };
    unscaled->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(6);

    // キーの操作と、時間の値・タイマーの表示をする役
    GameObject* demo = scene.CreateObject("TimeDemo");
    demo->AddComponent<TimeDemoController>(scaled, unscaled);
    return demo;
}
