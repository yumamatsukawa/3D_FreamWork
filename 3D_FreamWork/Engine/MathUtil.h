#pragma once
#include <DirectXMath.h>
#include <cmath>
#include "Transform.h"

using namespace DirectX;

// ─── XMFLOAT2 / XMFLOAT3 の演算子 ─────────────────────────────
// XMFLOAT2/3はDirectXの型なので、namespace DirectX の中に書くとどこからでも見つかる(ADL)
namespace DirectX {
    inline XMFLOAT2 operator+(XMFLOAT2 a, XMFLOAT2 b) { return { a.x + b.x, a.y + b.y }; }                  // 足し算
    inline XMFLOAT2 operator-(XMFLOAT2 a, XMFLOAT2 b) { return { a.x - b.x, a.y - b.y }; }                  // 引き算
    inline XMFLOAT2 operator*(XMFLOAT2 v, float s) { return { v.x * s, v.y * s }; }                         // 何倍かする
    inline XMFLOAT3 operator+(XMFLOAT3 a, XMFLOAT3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }       // 足し算
    inline XMFLOAT3 operator-(XMFLOAT3 a, XMFLOAT3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }       // 引き算
    inline XMFLOAT3 operator*(XMFLOAT3 v, float s) { return { v.x * s, v.y * s, v.z * s }; }                // 何倍かする
}

// 方向・距離・角度・乱数の計算をまとめたもの。角度はすべて「度」
namespace Math {
    // ─── ベクトル ─────────────────────────────
    inline float Length(XMFLOAT3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }   // 長さ
    inline float Length(XMFLOAT2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
    inline float DistanceSq(XMFLOAT3 a, XMFLOAT3 b) { XMFLOAT3 d = b - a; return d.x * d.x + d.y * d.y + d.z * d.z; }   // 距離の2乗(大小を比べるだけならsqrt不要で速い)
    inline float DistanceSq(XMFLOAT2 a, XMFLOAT2 b) { XMFLOAT2 d = b - a; return d.x * d.x + d.y * d.y; }
    inline float Distance(XMFLOAT3 a, XMFLOAT3 b) { return Length(b - a); }   // 2点の距離
    inline float Distance(XMFLOAT2 a, XMFLOAT2 b) { return Length(b - a); }

    // 長さ1にする(長さがほぼ0なら0ベクトルを返す。0で割らない)
    inline XMFLOAT3 Normalize(XMFLOAT3 v) {
        float len = Length(v);
        if (len < 1e-6f) return { 0.f, 0.f, 0.f };
        return v * (1.f / len);
    }
    inline XMFLOAT2 Normalize(XMFLOAT2 v) {
        float len = Length(v);
        if (len < 1e-6f) return { 0.f, 0.f };
        return v * (1.f / len);
    }

    inline XMFLOAT3 DirectionTo(XMFLOAT3 from, XMFLOAT3 to) { return Normalize(to - from); }   // fromからtoへの向き(長さ1)
    inline XMFLOAT2 DirectionTo(XMFLOAT2 from, XMFLOAT2 to) { return Normalize(to - from); }

    inline float Dot(XMFLOAT3 a, XMFLOAT3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }   // 内積(同じ向きなら+、逆なら-)
    inline XMFLOAT3 Cross(XMFLOAT3 a, XMFLOAT3 b) {                                            // 外積(2つに垂直な向き。Cross(上, 正面)=右)
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }
    inline XMFLOAT2 ToXY(XMFLOAT3 v) { return { v.x, v.y }; }   // x,yだけ取り出す(2D用)

    // ─── 補間・制限 ─────────────────────────────
    inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }                // aとbの間(t=0でa、t=1でb。tは0〜1に制限しない)
    inline XMFLOAT3 Lerp(XMFLOAT3 a, XMFLOAT3 b, float t) { return a + (b - a) * t; }     // 位置の補間
    inline float Clamp(float value, float minValue, float maxValue) {                       // minValue〜maxValueに収める
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    // currentからtargetへ、最大maxDistanceだけ近づく(行き過ぎない)
    inline XMFLOAT3 MoveTowards(XMFLOAT3 current, XMFLOAT3 target, float maxDistance) {
        XMFLOAT3 diff = target - current;
        float dist = Length(diff);
        if (dist <= maxDistance || dist < 1e-6f) return target;
        return current + diff * (maxDistance / dist);
    }

    // ─── 角度(すべて度) ─────────────────────────────
    // currentからtargetへの角度の差(-180〜180。350度と10度の差は20度)
    inline float DeltaAngle(float current, float target) {
        float delta = fmodf(target - current, 360.f);
        if (delta > 180.f) delta -= 360.f;
        if (delta < -180.f) delta += 360.f;
        return delta;
    }

    // currentからtargetへ、近い方向に最大maxDeltaだけ回る(回転速度の上限)
    inline float MoveTowardsAngle(float current, float target, float maxDelta) {
        float delta = DeltaAngle(current, target);
        if (fabsf(delta) <= maxDelta) return target;
        return current + (delta > 0.f ? maxDelta : -maxDelta);
    }

    // toの方を向くrotate.zの値(2D。rotate.z=0で上(Y+)、増えると左回り)。Forward2Dの逆
    inline float AngleToTarget2D(XMFLOAT2 from, XMFLOAT2 to) {
        return XMConvertToDegrees(atan2f(-(to.x - from.x), to.y - from.y));
    }

    // toの方を向くrotate.yの値(3Dの水平の向き。rotate.y=0で+Z、増えると+X側へ回る)。Forward3Dの逆
    inline float AngleToTargetY(XMFLOAT3 from, XMFLOAT3 to) {
        return XMConvertToDegrees(atan2f(to.x - from.x, to.z - from.z));
    }

    // 2Dの正面(rotate.z=0で(0,1))。見た目の回転と同じ向き
    inline XMFLOAT2 Forward2D(const Transform& t) {
        float rad = XMConvertToRadians(-t.rotate.z);
        return { sinf(rad), cosf(rad) };
    }

    // 3Dの水平の正面(rotate.yだけを使う。rotate.y=0で(0,0,1))。右は Cross({0,1,0}, 正面) で求める
    inline XMFLOAT3 Forward3D(const Transform& t) {
        float rad = XMConvertToRadians(t.rotate.y);
        return { sinf(rad), 0.f, cosf(rad) };
    }

    // ─── 乱数(MathUtil.cpp) ─────────────────────────────
    float RandomRange(float minValue, float maxValue);   // minValue〜maxValueのランダムな小数
    int RandomRange(int minValue, int maxValue);         // minValue〜maxValueのランダムな整数(maxValueも含む)
    XMFLOAT2 RandomOnCircle(float radius);               // 原点を中心とした半径radiusの円周上のランダムな点
}
