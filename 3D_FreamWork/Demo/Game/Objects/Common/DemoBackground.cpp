#include "DemoBackground.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../Components/Common/BackgroundController.h"
#include <climits>

GameObject* CreateDemoBackground(Scene& scene) {
    GameObject* background = scene.CreateObject("Background");
    // 必ず一番最初に描く(=他の全てのスプライトの奥に表示される)
    background->SetDrawPriority(INT_MIN);

    // タイルの画像と、1枚の大きさ
    background->AddComponent<BackgroundController>(L"Assets/bg_tile.png", 256.0f);
    return background;
}
