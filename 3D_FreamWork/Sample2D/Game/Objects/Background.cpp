#include "Background.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/BackgroundController.h"
#include <climits>

GameObject* CreateBackground(Scene& scene) {
    GameObject* background = scene.CreateObject("Background");
    // 必ず一番最初に描く(=他の全てのスプライトの奥に表示される)
    background->SetDrawPriority(INT_MIN);

    // タイルの画像と、1枚の大きさ
    background->AddComponent<BackgroundController>(L"Assets/bg_tile.png", 256.0f);
    return background;
}
