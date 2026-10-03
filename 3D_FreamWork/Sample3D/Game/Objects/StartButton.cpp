#include "StartButton.h"
#include "Button.h"
#include "../Scenes/GameScene.h"
#include "../../../Engine/SceneManager.h"

GameObject* CreateStartButton(Scene& scene) {
    // ここでは「どこに置いて、押されたら何をするか」だけを決める
    return CreateButton(scene, L"START", { 0.f, -150.f }, [] {
        SceneManager::Get().ChangeScene<GameScene>();
    });
}
