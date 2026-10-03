#include "HUDController.h"
#include "GameManager.h"
#include "PlayerController.h"
#include "../../../Engine/Text.h"
#include <cstdio>

void HUDController::Draw() {
    wchar_t buf[64];

    // スコア
    if (gameManager) {
        swprintf_s(buf, L"Score: %d", gameManager->GetScore());
        Text::Draw(buf, -560.f, 320.f, 28.f);
    }

    // HP
    if (player) {
        swprintf_s(buf, L"HP: %d / %d", player->GetHp(), player->GetMaxHp());
        Text::Draw(buf, -560.f, 280.f, 28.f, { 1.0f, 0.4f, 0.4f, 1.0f });
    }

    // ゲームオーバー
    if (gameManager && gameManager->IsGameOver()) {
        Text::Draw(L"GAME OVER", 0.f, 40.f, 64.f, { 1.0f, 0.2f, 0.2f, 1.0f });
        Text::Draw(L"press SPACE to return to Title", 0.f, -30.f, 24.f);
    }
}
