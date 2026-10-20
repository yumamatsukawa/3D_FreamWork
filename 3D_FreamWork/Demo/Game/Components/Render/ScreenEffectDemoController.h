#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// ScreenEffectデモの操作役。1〜4キーで画面全体の演出(Flash・Vignette・Fade黒・Fade白)を出し、IsFading の状態を文字で出す。
// 全体の設定(暗転の濃さ)を変えるので、出る時(Uninit)に必ず明るい状態へ戻す
class ScreenEffectDemoController : public Component {
private:
    static constexpr float highlightTime = 0.5f;    // 押したキーの行を光らせておく時間(秒)

    int lastPressed = -1;        // 最後に押した演出(0〜3。まだ押していなければ -1)
    float highlightTimer = 0.f;  // 光らせる残りの時間(秒)
    int completeCount = 0;       // Fade の onComplete が呼ばれた回数

    void StartFade(DirectX::XMFLOAT4 color);   // 暗転して、暗転し終わったら(onComplete)明転する

public:
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
