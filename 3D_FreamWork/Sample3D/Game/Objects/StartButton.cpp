#include "StartButton.h"
#include "Button.h"
#include "../Scenes/GameScene.h"
#include "../../../Engine/SceneManager.h"

GameObject* CreateStartButton(Scene& scene) {
    // ここでは「どこに置いて、押されたら何をするか」だけを決める
    Transform tf = {};
    tf.position = { 0.f, -150.f , 0.f};
    tf.scale = { 320.f, 96.f, 1.f };
    GameObject* startButton = CreateButton(scene, L"Start", tf, []{
        SceneManager::Get().ChangeScene<GameScene>();
        });

    return startButton;
}
