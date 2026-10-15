#include "StartButton.h"
#include "Button.h"
#include "../Scenes/GameScene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SceneManager.h"
#include "../../../Engine/TweenComponent.h"

GameObject* CreateStartButton(Scene& scene) {
    // ここでは「どこに置いて、押されたら何をするか」だけを決める
    Transform tf = {};
    tf.position = { 0.f, -150.f , 0.f};
    tf.scale = { 320.f, 96.f, 1.f };
    GameObject* startButton = CreateButton(scene, L"Start", tf, []{
        // 黒で0.5秒かけて暗転 → GameSceneへ切り替え → 0.5秒かけて明転
        // (切り替え中にもう一度押されても、SceneManagerが無視するので二重に切り替わらない)
        SceneManager::Get().ChangeScene<GameScene>(FadeTransition());
        });

    // 出る時の演出: 大きさ0から本来の大きさへ、0.4秒でぴょこっと大きくする
    // (クリック判定もtransform.scaleを使うので、大きくなる途中も見た目と判定が合う)
    DirectX::XMFLOAT3 size = startButton->transform.scale;
    startButton->transform.scale = { 0.f, 0.f, size.z };
    startButton->AddComponent<TweenComponent>()->ScaleTo(size, 0.4f, Ease::OutBack);

    return startButton;
}
