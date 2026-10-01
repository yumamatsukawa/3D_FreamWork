#include "HUD.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/HUDController.h"

GameObject* CreateHUD(Scene& scene, GameManager* gameManager, PlayerController* player) {
    GameObject* hud = scene.CreateObject("HUD");
    HUDController* controller = hud->AddComponent<HUDController>();
    controller->Setup(gameManager, player);
    return hud;
}
