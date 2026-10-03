#pragma once
#include "../../../Engine/Component.h"

class GameManager;
class PlayerController;

// スコア・HP・GAME OVERの表示を担当する(値は自分で持たず、持ち主から読むだけ)
class HUDController : public Component {
private:
    GameManager* gameManager = nullptr;   // スコアの持ち主
    PlayerController* player = nullptr;   // HPの持ち主

public:
    // 表示する値の持ち主を、CreateHUD()から渡してもらう
    void Setup(GameManager* manager, PlayerController* playerController) {
        gameManager = manager;
        player = playerController;
    }

    void Draw() override;
};
