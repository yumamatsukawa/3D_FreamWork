#include "HUDController.h"
#include "GameManager.h"
#include "PlayerController.h"
#include "../../../Engine/UI/Text.h"
#include "../../../Engine/UI/Anchor.h"
#include "../../../Engine/Render/Visibility.h"
#include <cstdio>

namespace {
    // 文字の位置(画面の隅・中央からのずらし量。画面座標は「中央が(0,0)、Y+=上」なので、下へずらす時はマイナス)
    constexpr DirectX::XMFLOAT2 scoreOffset = { 80.f, -40.f };         // 左上から: Score
    constexpr DirectX::XMFLOAT2 highScoreOffset = { 80.f, -75.f };     // 左上から: High Score
    constexpr DirectX::XMFLOAT2 hpOffset = { 80.f, -115.f };           // 左上から: HP(この下にHPゲージが出る。Objects/HUD.cpp)
    constexpr DirectX::XMFLOAT2 culledOffset = { 80.f, -165.f };       // 左上から: Culled(HPゲージの下)
    constexpr DirectX::XMFLOAT2 gameOverOffset = { 0.f, 40.f };        // 中央から: GAME OVER
    constexpr DirectX::XMFLOAT2 returnHintOffset = { 0.f, -30.f };     // 中央から: press SPACE ...
}

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
        DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, scoreOffset);
        Text::Draw(buf, pos.x, pos.y, style);

        swprintf_s(buf, L"High Score: %d", gameManager->GetHighScore());
        style.size = 22.f;
        style.color = { 1.0f, 0.9f, 0.4f, 1.0f };
        pos = GetAnchoredPosition(Anchor::TopLeft, highScoreOffset);
        Text::Draw(buf, pos.x, pos.y, style);
    }

    // HP
    if (player) {
        swprintf_s(buf, L"HP: %d / %d", player->GetHp(), player->GetMaxHp());
        style.size = 28.f;
        style.color = { 1.0f, 0.4f, 0.4f, 1.0f };
        DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, hpOffset);
        Text::Draw(buf, pos.x, pos.y, style);
    }

    // 画面の外なので描かずに省いた数(Visibility。ちゃんと省けているかの確認用)
    swprintf_s(buf, L"Culled: %d", Visibility::GetCulledCount());
    style.size = 20.f;
    style.color = { 0.7f, 0.9f, 1.0f, 1.0f };
    {
        DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, culledOffset);
        Text::Draw(buf, pos.x, pos.y, style);
    }

    // ゲームオーバー(画面の中央に、中央寄せで)
    if (gameManager && gameManager->IsGameOver()) {
        style.align = TextAlign::Center;
        style.size = 64.f;
        style.color = { 1.0f, 0.2f, 0.2f, 1.0f };
        DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::Center, gameOverOffset);
        Text::Draw(L"GAME OVER", pos.x, pos.y, style);

        style.size = 24.f;
        style.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        pos = GetAnchoredPosition(Anchor::Center, returnHintOffset);
        Text::Draw(L"press SPACE to return to Title", pos.x, pos.y, style);
    }
}
