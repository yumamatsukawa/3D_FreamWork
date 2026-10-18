#pragma once
#include <cmath>
#include "../Core/MathUtil.h"

// イージング関数(動きの「速さの変化」)をまとめたもの。式は easings.net と同じ。
// すべて t(進み具合 0〜1)を受け取って、0〜1の値を返す(t=0で0、t=1で1)。
// OutBack / OutElastic は途中で0〜1の範囲を少しはみ出す(行き過ぎてから戻る・揺れる)。
// t が0〜1の外で渡された時は、0〜1に収めて(Clampして)から計算する(t<0は0、t>1は1を返す)
//
// 使い方:
//   float v = Math::Lerp(from, to, Ease::OutQuad(t));   // fromからtoへ、ゆっくり止まる
namespace Ease {
    // 一定の速さ
    inline float Linear(float t) {
        return Math::Clamp(t, 0.f, 1.f);
    }

    // ゆっくり始まる
    inline float InQuad(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        return t * t;
    }

    // ゆっくり止まる
    inline float OutQuad(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        return 1.f - (1.f - t) * (1.f - t);
    }

    // ゆっくり始まって、ゆっくり止まる
    inline float InOutQuad(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        if (t < 0.5f) return 2.f * t * t;
        float u = -2.f * t + 2.f;
        return 1.f - u * u / 2.f;
    }

    // OutQuadより強くゆっくり止まる
    inline float OutCubic(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        float u = 1.f - t;
        return 1.f - u * u * u;
    }

    // 少し行き過ぎてから戻る(途中で1を超える)
    inline float OutBack(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        if (t <= 0.f) return 0.f;   // 式のままだと小数の誤差で0ぴったりにならないので、両端は決め打ち
        if (t >= 1.f) return 1.f;
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.f;
        float u = t - 1.f;
        return 1.f + c3 * u * u * u + c1 * u * u;
    }

    // ばねのように揺れて止まる(途中で0〜1をはみ出す)
    inline float OutElastic(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        if (t <= 0.f) return 0.f;
        if (t >= 1.f) return 1.f;
        const float c4 = (2.f * DirectX::XM_PI) / 3.f;
        return powf(2.f, -10.f * t) * sinf((t * 10.f - 0.75f) * c4) + 1.f;
    }

    // ボールのように跳ねて止まる
    inline float OutBounce(float t) {
        t = Math::Clamp(t, 0.f, 1.f);
        if (t <= 0.f) return 0.f;
        if (t >= 1.f) return 1.f;
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
        if (t < 1.f / d1) {
            return n1 * t * t;
        } else if (t < 2.f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        } else if (t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        } else {
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
    }
}

// イージング関数の型。例: EaseFunction ease = Ease::OutBack;
using EaseFunction = float(*)(float);
