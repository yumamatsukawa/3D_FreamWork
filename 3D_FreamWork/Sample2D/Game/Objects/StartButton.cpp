#include "StartButton.h"
#include "Button.h"
#include "../Scenes/GameScene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SceneManager.h"
#include "../../../Engine/TweenComponent.h"

GameObject* CreateStartButton(Scene& scene) {
    // ここでは「どこに置いて、押されたら何をするか」だけを決める
    GameObject* button = CreateButton(scene, L"START", { 0.f, -150.f }, [] {
        SceneManager::Get().ChangeScene<GameScene>();
    });

    // 出る時の演出: 大きさ0から本来の大きさへ、0.4秒でぴょこっと大きくする
    // (クリック判定もtransform.scaleを使うので、大きくなる途中も見た目と判定が合う)
    DirectX::XMFLOAT3 size = button->transform.scale;
    button->transform.scale = { 0.f, 0.f, size.z };
    button->AddComponent<TweenComponent>()->ScaleTo(size, 0.4f, Ease::OutBack);
    return button;
}
