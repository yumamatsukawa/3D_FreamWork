#pragma once
#include "../../../Engine/Component.h"

class GameManager;
class PlayerController;

// スコア・HP・ゲームオーバー表示など、画面のUI(HUD)だけを担当するComponent。
// 値は自分では持たず、持ち主(GameManager/PlayerController)からゲッターで毎フレーム読むだけ。
// こうしておくと「表示用のコピー」と「本物の値」がズレることが無い
class HUDController : public Component {
private:
    GameManager* gameManager = nullptr;
    PlayerController* player = nullptr;

public:
    // 表示に使う値の持ち主を、GameScene::Init()側から渡してもらう
    void Setup(GameManager* manager, PlayerController* playerController) {
        gameManager = manager;
        player = playerController;
    }

    void Draw() override;
};
