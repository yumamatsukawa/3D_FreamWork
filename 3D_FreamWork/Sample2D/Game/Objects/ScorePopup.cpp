#include "ScorePopup.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/TextRenderer.h"
#include "../../../Engine/TweenComponent.h"

namespace {
    constexpr float riseHeight = 60.f;   // 上へ動く距離(ワールド)
    constexpr float duration = 0.8f;     // 消えるまでの時間(秒)
}

GameObject* CreateScorePopup(Scene& scene, DirectX::XMFLOAT3 position) {
    GameObject* popup = scene.CreateObject("ScorePopup");
    popup->transform.position = position;

    // 見た目: 黄色の太字に黒い縁取り
    TextStyle style;
    style.size = 32.f;
    style.color = { 1.0f, 0.9f, 0.3f, 1.0f };
    style.fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");   // 同じファイルは2回読まない
    style.bold = true;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    auto* text = popup->AddComponent<TextRenderer>();
    text->SetText(L"+10");
    text->SetStyle(style);

    // 動き: 上へ動きながら、文字をだんだん透明にする。消え終わったら自分を消す
    auto* tween = popup->AddComponent<TweenComponent>();
    tween->MoveTo({ position.x, position.y + riseHeight, position.z }, duration, Ease::OutQuad);
    tween->TweenValue(1.0f, 0.0f, duration, [text](float alpha) {
        DirectX::XMFLOAT4 color = text->GetStyle().color;
        color.w = alpha;
        text->SetColor(color);
    }, Ease::InQuad).SetOnComplete([popup]() {
        popup->Destroy();
    });

    return popup;
}
