#include "ScreenEffect.h"
#include "../UI/Text.h"
#include "Graphics.h"
#include "../Core/MathUtil.h"
#include "../Animation/Ease.h"
#include <cmath>
#include <utility>

namespace ScreenEffect {
    namespace {
        // ビネット(画面の端の色)の形。楕円の中心は画面の中央
        constexpr float vignetteRadiusScale = 1.2f;  // 楕円の半径(画面の幅・高さの半分に対する倍率。1より大きいと、辺の真ん中は少し薄くなり、四隅が一番濃くなる)
        constexpr float vignetteInnerRatio = 0.5f;   // 中央の透明な部分の大きさ(楕円の半径に対する割合。ここから外へ向かってだんだん濃くなる)

        // フラッシュ
        XMFLOAT4 flashColor{};       // 色(wは光り始めの濃さ)
        float flashDuration = 0.f;   // 光っている時間(秒)。0なら光っていない
        float flashElapsed = 0.f;    // 光り始めてからの時間(秒)

        // ビネット
        XMFLOAT4 vignetteColor{};       // 色(wは一番濃い時の濃さ)
        float vignetteDuration = 0.f;   // 点滅している時間(秒)。0なら出ていない
        float vignetteElapsed = 0.f;    // 点滅し始めてからの時間(秒)
        int vignetteBlinkCount = 1;     // 点滅する回数
        bool vignetteColorChanged = false;  // 色が変わったか(Drawでグラデーションを作り直すため)

        // フェード(シーンをまたいで残る)
        XMFLOAT3 fadeColor{};        // 色
        float fadeAlpha = 0.f;       // 今の濃さ(0〜1)
        float fadeFrom = 0.f;        // 変わり始めた時の濃さ
        float fadeTo = 0.f;          // 目標の濃さ
        float fadeDuration = 0.f;    // 変わり終わるまでの時間(秒)
        float fadeElapsed = 0.f;     // 変わり始めてからの時間(秒)
        bool fading = false;         // 変わっている途中か
        std::function<void()> fadeOnComplete;  // 変わり終わった時に呼ぶ関数

        // Direct2Dのブラシ(毎フレーム作り直さず、最初に要った時に作って使い回す。Uninitで解放)
        ID2D1RenderTarget* brushTarget = nullptr;          // ブラシを作った描画先(違う描画先では使えないため覚えておく)
        ID2D1SolidColorBrush* fillBrush = nullptr;         // 画面全体の塗りつぶし用(フラッシュ・フェード。色は描く時に変える)
        ID2D1RadialGradientBrush* vignetteBrush = nullptr; // ビネット用(中央が透明で、外ほど色が濃い放射状のグラデーション)

        void ReleaseBrushes() {
            if (fillBrush) { fillBrush->Release(); fillBrush = nullptr; }
            if (vignetteBrush) { vignetteBrush->Release(); vignetteBrush = nullptr; }
            brushTarget = nullptr;
        }

        // 今のフラッシュの濃さ(だんだん弱まり、終わったら0)
        float GetFlashAlpha() {
            if (flashDuration <= 0.f || flashElapsed >= flashDuration) return 0.f;
            return flashColor.w * (1.f - Ease::OutQuad(flashElapsed / flashDuration));
        }

        // 今のビネットの濃さ(0から濃くなって0に戻る、をblinkCount回くり返す)
        float GetVignetteAlpha() {
            if (vignetteDuration <= 0.f || vignetteElapsed >= vignetteDuration) return 0.f;
            float phase = vignetteElapsed / vignetteDuration * (float)vignetteBlinkCount;
            return vignetteColor.w * fabsf(sinf(DirectX::XM_PI * phase));
        }

        // 塗りつぶし用のブラシを用意する(無ければ作る)
        bool PrepareFillBrush(ID2D1RenderTarget* target) {
            if (fillBrush) return true;
            target->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 0.f), &fillBrush);
            return fillBrush != nullptr;
        }

        // ビネット用のブラシを用意する(無いか、色が変わった時だけ作り直す)
        bool PrepareVignetteBrush(ID2D1RenderTarget* target) {
            if (vignetteBrush && !vignetteColorChanged) return true;
            if (vignetteBrush) { vignetteBrush->Release(); vignetteBrush = nullptr; }
            vignetteColorChanged = false;

            // グラデーション: 中央〜vignetteInnerRatioまでは透明、そこから外へだんだん濃くなる。
            // 濃さ(color.w)はここでは1にしておき、描く時にブラシの不透明度(SetOpacity)で変える
            const float r = vignetteColor.x, g = vignetteColor.y, b = vignetteColor.z;
            D2D1_GRADIENT_STOP stops[] = {
                { 0.f,                D2D1::ColorF(r, g, b, 0.f) },
                { vignetteInnerRatio, D2D1::ColorF(r, g, b, 0.f) },
                { 1.f,                D2D1::ColorF(r, g, b, 1.f) },
            };
            ID2D1GradientStopCollection* stopCollection = nullptr;
            target->CreateGradientStopCollection(stops, ARRAYSIZE(stops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stopCollection);
            if (!stopCollection) return false;

            // 中心・半径は描く時に画面の大きさから決めるので、ここでは仮の値
            target->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0.f, 0.f), 1.f, 1.f),
                stopCollection, &vignetteBrush);
            stopCollection->Release();   // ブラシの方が中で持っているので、ここで手放してよい
            return vignetteBrush != nullptr;
        }
    }

    void Flash(XMFLOAT4 color, float duration) {
        if (duration <= 0.f) {   // すぐに目標(0)にする=光らない
            flashDuration = 0.f;
            flashElapsed = 0.f;
            return;
        }
        flashColor = color;
        flashDuration = duration;
        flashElapsed = 0.f;
    }

    void Vignette(XMFLOAT4 color, float duration, int blinkCount) {
        if (duration <= 0.f) {   // すぐに目標(0)にする=出さない
            vignetteDuration = 0.f;
            vignetteElapsed = 0.f;
            return;
        }
        // 色(x,y,z)が変わった時だけ、グラデーションを作り直してもらう(濃さwはSetOpacityで変えるので作り直さない)
        if (color.x != vignetteColor.x || color.y != vignetteColor.y || color.z != vignetteColor.z) {
            vignetteColorChanged = true;
        }
        vignetteColor = color;
        vignetteDuration = duration;
        vignetteElapsed = 0.f;
        vignetteBlinkCount = (blinkCount < 1) ? 1 : blinkCount;
    }

    void Fade(XMFLOAT4 color, float toAlpha, float duration, std::function<void()> onComplete) {
        fadeColor = { color.x, color.y, color.z };
        fadeFrom = fadeAlpha;
        fadeTo = Math::Clamp(toAlpha, 0.f, 1.f);
        fadeDuration = duration;
        fadeElapsed = 0.f;
        fadeOnComplete = nullptr;   // 前のFadeの完了の関数は呼ばずに捨てる

        // 時間が0以下なら、すぐに目標の濃さにして、完了の関数もその場で呼ぶ
        if (duration <= 0.f) {
            fadeAlpha = fadeTo;
            fading = false;
            if (onComplete) onComplete();
            return;
        }
        fadeOnComplete = std::move(onComplete);
        fading = true;
    }

    bool IsFading() {
        return fading;
    }

    void Stop() {
        // Flash と Vignette だけを消す(Fade は濃さも進み具合もそのまま)
        flashDuration = 0.f;
        flashElapsed = 0.f;
        vignetteDuration = 0.f;
        vignetteElapsed = 0.f;
    }

    void OnSceneChanged() {
        Stop();
        // Fade の濃さと進み具合は残す(暗転→切り替え→次のシーンで明転、のため)。
        // 完了の関数は、消えたシーンのオブジェクトを捕まえているかもしれないので呼ばずに捨てる
        fadeOnComplete = nullptr;
    }

    void Update(float unscaledDt) {
        // フラッシュ: 終わったら光っていない状態に戻す
        if (flashDuration > 0.f) {
            flashElapsed += unscaledDt;
            if (flashElapsed >= flashDuration) { flashDuration = 0.f; flashElapsed = 0.f; }
        }

        // ビネット: 終わったら出ていない状態に戻す
        if (vignetteDuration > 0.f) {
            vignetteElapsed += unscaledDt;
            if (vignetteElapsed >= vignetteDuration) { vignetteDuration = 0.f; vignetteElapsed = 0.f; }
        }

        // フェード: 目標の濃さへ近づける。着いたら完了の関数を1回だけ呼ぶ
        if (fading) {
            fadeElapsed += unscaledDt;
            if (fadeElapsed < fadeDuration) {
                fadeAlpha = Math::Lerp(fadeFrom, fadeTo, Ease::Linear(fadeElapsed / fadeDuration));
            } else {
                fadeAlpha = fadeTo;
                fading = false;
                // 先に取り出して空にしてから呼ぶ(中でまたFadeを呼んでも、新しい方を消さないように)
                std::function<void()> onComplete = std::move(fadeOnComplete);
                fadeOnComplete = nullptr;
                if (onComplete) onComplete();
            }
        }
    }

    void Draw() {
        ID2D1RenderTarget* target = Text::GetRenderTarget();
        if (!target) return;

        float vignetteAlpha = GetVignetteAlpha();
        float flashAlpha = GetFlashAlpha();
        if (vignetteAlpha <= 0.f && flashAlpha <= 0.f && fadeAlpha <= 0.f) return;   // 何も出ていない

        // 描画先が変わっていたら、前の描画先で作ったブラシは使えないので作り直す
        if (target != brushTarget) {
            ReleaseBrushes();
            brushTarget = target;
            vignetteColorChanged = true;
        }

        const float width = Graphics::width;
        const float height = Graphics::height;
        const D2D1_RECT_F screenRect = D2D1::RectF(0.f, 0.f, width, height);

        // 重なる順番は下から ビネット → フラッシュ → フェード(Direct2Dは後に描いた物が上になる)

        // ビネット: 画面の中央を中心にした楕円のグラデーションで、画面全体を塗る
        if (vignetteAlpha > 0.f && PrepareVignetteBrush(target)) {
            vignetteBrush->SetCenter(D2D1::Point2F(width / 2.f, height / 2.f));
            vignetteBrush->SetRadiusX(width / 2.f * vignetteRadiusScale);
            vignetteBrush->SetRadiusY(height / 2.f * vignetteRadiusScale);
            vignetteBrush->SetOpacity(vignetteAlpha);
            target->FillRectangle(screenRect, vignetteBrush);
        }

        // フラッシュ
        if (flashAlpha > 0.f && PrepareFillBrush(target)) {
            fillBrush->SetColor(D2D1::ColorF(flashColor.x, flashColor.y, flashColor.z, flashAlpha));
            target->FillRectangle(screenRect, fillBrush);
        }

        // フェード
        if (fadeAlpha > 0.f && PrepareFillBrush(target)) {
            fillBrush->SetColor(D2D1::ColorF(fadeColor.x, fadeColor.y, fadeColor.z, fadeAlpha));
            target->FillRectangle(screenRect, fillBrush);
        }
    }

    void Uninit() {
        ReleaseBrushes();

        // 状態も最初に戻す(完了の関数が持っている物も手放す)
        flashDuration = 0.f;
        flashElapsed = 0.f;
        vignetteDuration = 0.f;
        vignetteElapsed = 0.f;
        fadeAlpha = 0.f;
        fading = false;
        fadeOnComplete = nullptr;
    }
}
