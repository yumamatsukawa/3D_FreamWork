#pragma once
#include "../../../Engine/Component.h"
#include <string>

class GameManager;
class PlayerController;

// スコア・ハイスコア・HP・GAME OVERの表示を担当する(値は自分で持たず、持ち主から読むだけ)
class HUDController : public Component {
private:
    GameManager* gameManager = nullptr;   // スコアの持ち主(ハイスコアもここから読む)
    PlayerController* player = nullptr;   // HPの持ち主
    std::wstring fontName;                // 文字のフォント(InitでText::LoadFontした名前)

public:
    // 表示する値の持ち主を、CreateHUD()から渡してもらう
    void Setup(GameManager* manager, PlayerController* playerController) {
        gameManager = manager;
        player = playerController;
    }

    void Init() override;
    void Draw() override;
};
