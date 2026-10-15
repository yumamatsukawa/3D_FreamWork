#include "HUDController.h"
#include "GameManager.h"
#include "PlayerController.h"
#include "../../../Engine/Text.h"
#include <cstdio>

void HUDController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、シーンに入る度に呼んでよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

void HUDController::Draw() {
    wchar_t buf[64];

    // 文字の見た目: 黒い縁取り。左上の文字は左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;

    // スコアとハイスコア
    if (gameManager) {
        swprintf_s(buf, L"Score: %d", gameManager->GetScore());
        style.size = 28.f;
        style.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Text::Draw(buf, -560.f, 320.f, style);

        swprintf_s(buf, L"High Score: %d", gameManager->GetHighScore());
        style.size = 22.f;
        style.color = { 1.0f, 0.9f, 0.4f, 1.0f };
        Text::Draw(buf, -560.f, 285.f, style);
    }

    // HP
    if (player) {
        swprintf_s(buf, L"HP: %d / %d", player->GetHp(), player->GetMaxHp());
        style.size = 28.f;
        style.color = { 1.0f, 0.4f, 0.4f, 1.0f };
        Text::Draw(buf, -560.f, 245.f, style);
    }

    // ゲームオーバー(画面の中央に、中央寄せで)
    if (gameManager && gameManager->IsGameOver()) {
        style.align = TextAlign::Center;
        style.size = 64.f;
        style.color = { 1.0f, 0.2f, 0.2f, 1.0f };
        Text::Draw(L"GAME OVER", 0.f, 40.f, style);

        style.size = 24.f;
        style.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Text::Draw(L"press SPACE to return to Title", 0.f, -30.f, style);
    }
}
