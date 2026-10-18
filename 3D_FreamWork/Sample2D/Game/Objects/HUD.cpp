#include "HUD.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/UI/GaugeRenderer.h"
#include "../Components/HUDController.h"
#include "../Components/PlayerController.h"

GameObject* CreateHUD(Scene& scene, GameManager* gameManager, PlayerController* player) {
    GameObject* hud = scene.CreateObject("HUD");
    // 表示と、表示する値の持ち主
    HUDController* controller = hud->AddComponent<HUDController>();
    controller->Setup(gameManager, player);

    // HPゲージ(HPの文字の下、画面の左上に固定。値は持たず、PlayerControllerのHPを毎フレーム読んで表示するだけ)
    GaugeRenderer* gauge = hud->AddComponent<GaugeRenderer>();
    gauge->SetAnchor(Anchor::TopLeft, { 80.f, -135.f });
    gauge->SetSize(240.f, 16.f);
    gauge->SetColors({ 1.0f, 0.4f, 0.4f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.6f });
    gauge->SetDelayedBar(true, { 1.0f, 0.9f, 0.4f, 1.0f });   // 減った分が少し遅れて縮む
    if (player) {
        gauge->SetValueSource(
            [player] { return static_cast<float>(player->GetHp()); },
            [player] { return static_cast<float>(player->GetMaxHp()); });
    }
    return hud;
}
