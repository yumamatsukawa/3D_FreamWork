#include "MathDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoMarker.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include <cmath>
#include <cstdio>

namespace {
    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4;

    // 印の色
    constexpr XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr XMFLOAT4 faint = { 1.0f, 1.0f, 1.0f, 0.35f };
    constexpr XMFLOAT4 blue = { 0.4f, 0.65f, 1.0f, 1.0f };
    constexpr XMFLOAT4 red = { 1.0f, 0.45f, 0.45f, 1.0f };
    constexpr XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
    constexpr XMFLOAT4 green = { 0.5f, 1.0f, 0.5f, 1.0f };

    // 文字の位置
    constexpr float menuY = 190.f;        // 関数の名前の並び(説明の板の下)
    constexpr float infoX = -610.f;       // 説明の文字の左端
    constexpr float infoY = 152.f;        // 説明の1行目
    constexpr float infoGap = 28.f;       // 行の間隔

    // 2D版の置き場所
    constexpr XMFLOAT3 lineA = { -300.f, -70.f, 0.f };     // Lerpの始まりの点
    constexpr XMFLOAT3 lineB = { 300.f, -70.f, 0.f };      // Lerpの終わりの点
    constexpr float clampMin = -200.f;                      // Clampの下限
    constexpr float clampMax = 200.f;                       // Clampの上限
    constexpr XMFLOAT3 angleCenter = { 0.f, -80.f, 0.f };   // 角度・距離の中心
    constexpr float histogramLeft = -520.f;                 // 分布の箱の左端
    constexpr float histogramBase = -190.f;                 // 分布の箱の底
    constexpr float binWidth = 30.f;                        // 箱1つの幅
    constexpr float histogramHeight = 150.f;                // 一番多い箱の高さ
    constexpr XMFLOAT2 scatterCenter = { 260.f, -90.f };    // 円周上の点の中心
    constexpr float scatterRadius = 100.f;                  // 円の半径
    constexpr size_t scatterPerFrame = 2;                   // 1フレームに足す円周上の点の数

    // 3D版の置き場所
    constexpr XMFLOAT3 origin3D = { 0.f, 40.f, 300.f };     // ベクトルの根元
    constexpr float vectorLength = 180.f;                   // 内積・外積に使うベクトルの長さ
    constexpr float orbitRadius = 200.f;                    // 目標の点が回る円の半径
    constexpr float followSpeed = 150.f;                    // MoveTowardsで近づく速さ(1秒あたり)
    constexpr float vectorSpeed = 120.f;                    // キーでベクトルを動かす速さ(1秒あたり)

    // 3D版: 目標の点(origin3Dの周りを回る)
    XMFLOAT3 GetOrbitTarget(float time) {
        return { origin3D.x + cosf(time * 0.8f) * orbitRadius, origin3D.y, origin3D.z + sinf(time * 0.8f) * orbitRadius };
    }
}

void MathDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;
    elapsed += dt;

    // 1〜4: 見せる関数を選ぶ
    for (int i = 0; i < 4; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) selected = i;
    }

    if (mode == DemoMode::Mode3D) Update3D(dt);
    else Update2D(dt);
}

void MathDemoController::Update2D(float dt) {
    const XMFLOAT2 mouse = Input::GetMouseWorldPosition();

    // 3: 腕は、マウスの方へ向く角度に向かって、1秒に180度まで回る
    if (selected == 2) {
        const float target = Math::AngleToTarget2D({ angleCenter.x, angleCenter.y }, mouse);
        armAngle = Math::MoveTowardsAngle(armAngle, target, 180.f * dt);
    }

    // 4: 乱数を足していく(クリックで数え直す。SPACEでサイコロ)
    if (selected == 3) {
        if (Input::GetKeyDown(MOUSE_LEFT)) {
            for (int& count : histogram) count = 0;
            scatter.clear();
        }
        if (Input::GetKeyDown(KEY_SPACE)) diceResult = Math::RandomRange(1, 6);

        for (int i = 0; i < 3; i++) {
            const int bin = (int)(Math::RandomRange(0.f, 1.f) * (float)binCount);
            histogram[bin < binCount ? bin : binCount - 1]++;
        }
        for (size_t i = 0; i < scatterPerFrame; i++) {
            scatter.push_back(Math::RandomOnCircle(scatterRadius));
            if (scatter.size() > maxScatter) scatter.erase(scatter.begin());
        }
    }
}

void MathDemoController::Update3D(float dt) {
    // 3: 矢印キー(X・Z)とW・S(Y)でベクトルを動かす
    if (selected == 2) {
        float moveX = 0.f, moveY = 0.f, moveZ = 0.f;
        if (Input::GetKeyPress(KEY_RIGHT)) moveX += 1.f;
        if (Input::GetKeyPress(KEY_LEFT)) moveX -= 1.f;
        if (Input::GetKeyPress(KEY_W)) moveY += 1.f;
        if (Input::GetKeyPress(KEY_S)) moveY -= 1.f;
        if (Input::GetKeyPress(KEY_UP)) moveZ += 1.f;
        if (Input::GetKeyPress(KEY_DOWN)) moveZ -= 1.f;
        userVector.x = Math::Clamp(userVector.x + moveX * vectorSpeed * dt, -250.f, 250.f);
        userVector.y = Math::Clamp(userVector.y + moveY * vectorSpeed * dt, -250.f, 250.f);
        userVector.z = Math::Clamp(userVector.z + moveZ * vectorSpeed * dt, -250.f, 250.f);
    }

    // 4: 点は、回る目標に向かって、1秒に followSpeed まで近づく
    if (selected == 3) follower = Math::MoveTowards(follower, GetOrbitTarget(elapsed), followSpeed * dt);
}

void MathDemoController::DrawMenu() const {
    // 関数の名前の並び(今選んでいるものを黄色にする)
    static const wchar_t* const names2D[4] = { L"1 Lerp", L"2 Clamp", L"3 Angle・Distance", L"4 Random" };
    static const wchar_t* const names3D[4] = { L"1 Lerp", L"2 Dot・Cross", L"3 Normalize", L"4 MoveTowards" };
    const wchar_t* const* names = (mode == DemoMode::Mode3D) ? names3D : names2D;

    float x = infoX;
    for (int i = 0; i < 4; i++) {
        const TextStyle style = MakeDemoTextStyle(22.f, (i == selected) ? yellow : XMFLOAT4{ 0.8f, 0.8f, 0.8f, 1.0f });
        Text::Draw(names[i], x, menuY, style);
        x += Text::Measure(names[i], style).x + 34.f;
    }
}

void MathDemoController::Draw() {
    DrawMenu();
    if (mode == DemoMode::Mode3D) Draw3D();
    else Draw2D();
}

void MathDemoController::Draw2D() const {
    const XMFLOAT2 mouse = Input::GetMouseWorldPosition();
    wchar_t buf[128];
    float y = infoY;

    if (selected == 0) {
        // Lerp: マウスのXから割合 t を決める(tが0〜1の外では、Lerpは線の外へ延びる。Clampは両端で止まる)
        const float t = (mouse.x - lineA.x) / (lineB.x - lineA.x);
        const float clampedT = Math::Clamp(t, 0.f, 1.f);
        swprintf_s(buf, L"t = %.2f   (マウスの横の位置を、a〜b の間の割合にしたもの)", t);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        Text::Draw(L"Lerp(a, b, t) → 黄色。t が 0〜1 の外でも延びる", infoX, y, MakeDemoTextStyle(20.f, yellow)); y -= infoGap;
        swprintf_s(buf, L"Clamp(t, 0, 1) = %.2f → 緑。a〜b の外へ出ない", clampedT);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, green));

        DrawDemoLine(mode, lineA, lineB, faint);
        DrawDemoPoint(mode, lineA, 14.f, blue);
        DrawDemoLabel(mode, lineA, L"a", -30.f);
        DrawDemoPoint(mode, lineB, 14.f, red);
        DrawDemoLabel(mode, lineB, L"b", -30.f);
        DrawDemoPoint(mode, Math::Lerp(lineA, lineB, t), 16.f, yellow);
        const XMFLOAT3 clamped = Math::Lerp(lineA, lineB, clampedT);
        DrawDemoPoint(mode, { clamped.x, clamped.y + 36.f, 0.f }, 16.f, green);
    }
    else if (selected == 1) {
        // Clamp: 値(マウスのX)を下限〜上限に収める
        const float value = mouse.x;
        const float clamped = Math::Clamp(value, clampMin, clampMax);
        swprintf_s(buf, L"value = %.0f   (マウスの横の位置)", value);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        swprintf_s(buf, L"Clamp(value, %.0f, %.0f) = %.0f", clampMin, clampMax, clamped);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, green)); y -= infoGap;
        Text::Draw(L"範囲の外に出た value は、いちばん近い端の値になります", infoX, y, MakeDemoTextStyle(20.f, { 0.8f, 0.8f, 0.8f, 1.0f }));

        const float barY = -80.f;
        DrawDemoLine(mode, { clampMin, barY, 0.f }, { clampMax, barY, 0.f }, faint);
        DrawDemoPoint(mode, { clampMin, barY, 0.f }, 14.f, blue);
        DrawDemoPoint(mode, { clampMax, barY, 0.f }, 14.f, blue);
        DrawDemoLabel(mode, { clampMin, barY, 0.f }, L"最小", -30.f);
        DrawDemoLabel(mode, { clampMax, barY, 0.f }, L"最大", -30.f);
        DrawDemoPoint(mode, { value, barY + 40.f, 0.f }, 16.f, red);
        DrawDemoLabel(mode, { value, barY + 40.f, 0.f }, L"value", 26.f);
        DrawDemoPoint(mode, { clamped, barY, 0.f }, 16.f, green);
    }
    else if (selected == 2) {
        // 距離・向き・長さ1の向き・角度の差
        const XMFLOAT2 center = { angleCenter.x, angleCenter.y };
        const XMFLOAT2 direction = Math::Normalize(mouse - center);
        const float target = Math::AngleToTarget2D(center, mouse);
        swprintf_s(buf, L"Distance(中心, マウス) = %.0f", Math::Distance(center, mouse));
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        swprintf_s(buf, L"Normalize(マウス - 中心) = (%.2f, %.2f) → 緑。長さ1の向き", direction.x, direction.y);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, green)); y -= infoGap;
        swprintf_s(buf, L"AngleToTarget2D = %.0f 度   DeltaAngle(腕, 目標) = %.0f 度 → 黄色の腕が回って追う", target, Math::DeltaAngle(armAngle, target));
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, yellow));

        DrawDemoLine(mode, angleCenter, { mouse.x, mouse.y, 0.f }, faint);
        DrawDemoPoint(mode, angleCenter, 14.f, blue);
        DrawDemoLine(mode, angleCenter, { angleCenter.x + direction.x * 90.f, angleCenter.y + direction.y * 90.f, 0.f }, green);
        Transform arm;
        arm.rotate.z = armAngle;
        const XMFLOAT2 armDirection = Math::Forward2D(arm);
        DrawDemoLine(mode, angleCenter, { angleCenter.x + armDirection.x * 130.f, angleCenter.y + armDirection.y * 130.f, 0.f }, yellow);
    }
    else {
        // 乱数: RandomRange(0, 1) の分布、RandomOnCircle の点、サイコロ
        swprintf_s(buf, L"RandomRange(1, 6) = %d   (SPACE でサイコロ)", diceResult);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        Text::Draw(L"左: RandomRange(0, 1) の結果を10個の箱に数えたもの(ほぼ同じ高さになる)", infoX, y, MakeDemoTextStyle(20.f, yellow)); y -= infoGap;
        Text::Draw(L"右: RandomOnCircle(100) の点(円周の上に並ぶ)   クリック: 数え直す", infoX, y, MakeDemoTextStyle(20.f, green));

        int maxCount = 1;
        for (int count : histogram) {
            if (count > maxCount) maxCount = count;
        }
        for (int i = 0; i < binCount; i++) {
            const float height = histogramHeight * (float)histogram[i] / (float)maxCount;
            Image::DrawRect({ histogramLeft + binWidth * ((float)i + 0.5f), histogramBase + height * 0.5f }, { binWidth - 4.f, height }, yellow);
        }
        Text::Draw(L"0", histogramLeft, histogramBase - 18.f, MakeDemoTextStyle(16.f, white, TextAlign::Center));
        Text::Draw(L"1", histogramLeft + binWidth * (float)binCount, histogramBase - 18.f, MakeDemoTextStyle(16.f, white, TextAlign::Center));

        for (const XMFLOAT2& point : scatter) {
            Image::DrawRect({ scatterCenter.x + point.x, scatterCenter.y + point.y }, { 5.f, 5.f }, green);
        }
    }
}

void MathDemoController::Draw3D() const {
    wchar_t buf[160];
    float y = infoY;

    // どの関数でも、根元に印を出す
    DrawDemoPoint(mode, origin3D, 12.f, white);

    if (selected == 0) {
        // Lerp: a と b の間を、t(行ったり来たり)の割合で進んだ点
        const XMFLOAT3 a = { -250.f, 0.f, 200.f };
        const XMFLOAT3 b = { 250.f, 150.f, 420.f };
        const float t = 0.5f + 0.5f * sinf(elapsed);
        swprintf_s(buf, L"t = %.2f   (時間で 0〜1 を行ったり来たりする)", t);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        Text::Draw(L"Lerp(a, b, t) → 黄色。位置(x, y, z)をまとめて補間する", infoX, y, MakeDemoTextStyle(20.f, yellow));

        DrawDemoLine(mode, a, b, faint);
        DrawDemoPoint(mode, a, 14.f, blue);
        DrawDemoLabel(mode, a, L"a");
        DrawDemoPoint(mode, b, 14.f, red);
        DrawDemoLabel(mode, b, L"b");
        DrawDemoPoint(mode, Math::Lerp(a, b, t), 18.f, yellow);
    }
    else if (selected == 1) {
        // 内積・外積: a は固定、b は水平に回る。外積は a と b の両方に垂直(ここでは上か下)
        const XMFLOAT3 a = { vectorLength, 0.f, 0.f };
        const XMFLOAT3 b = { cosf(elapsed * 0.8f) * vectorLength, 0.f, sinf(elapsed * 0.8f) * vectorLength };
        const float dot = Math::Dot(Math::Normalize(a), Math::Normalize(b));
        const XMFLOAT3 cross = Math::Normalize(Math::Cross(a, b));
        swprintf_s(buf, L"Dot(a, b) = %.2f   (長さ1にした向き同士。同じ向きで1、直角で0、逆向きで-1)", dot);
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        swprintf_s(buf, L"Cross(a, b) → 緑。a と b の両方に垂直な向き(今は %s)", cross.y >= 0.f ? L"上" : L"下");
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, green));

        DrawDemoLine(mode, origin3D, origin3D + a, blue);
        DrawDemoLabel(mode, origin3D + a, L"a");
        DrawDemoLine(mode, origin3D, origin3D + b, red);
        DrawDemoLabel(mode, origin3D + b, L"b");
        DrawDemoLine(mode, origin3D, origin3D + cross * 150.f, green);
    }
    else if (selected == 2) {
        // Normalize: 長さを1にする(向きはそのまま)
        const XMFLOAT3 unit = Math::Normalize(userVector);
        swprintf_s(buf, L"v = (%.0f, %.0f, %.0f)   Length(v) = %.0f   (矢印 W S キーで動かす)", userVector.x, userVector.y, userVector.z, Math::Length(userVector));
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, yellow)); y -= infoGap;
        swprintf_s(buf, L"Normalize(v) = (%.2f, %.2f, %.2f)   Length = %.2f → 緑(長さ1。見やすいよう150倍で描く)", unit.x, unit.y, unit.z, Math::Length(unit));
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f, green));

        DrawDemoLine(mode, origin3D, origin3D + userVector, yellow);
        DrawDemoLine(mode, origin3D, origin3D + unit * 150.f, green);
    }
    else {
        // MoveTowards と Distance: 点は回る目標を追いかける(1秒に決まった距離しか進めない)
        const XMFLOAT3 target = GetOrbitTarget(elapsed);
        swprintf_s(buf, L"Distance(点, 目標) = %.0f", Math::Distance(follower, target));
        Text::Draw(buf, infoX, y, MakeDemoTextStyle(20.f)); y -= infoGap;
        Text::Draw(L"MoveTowards(点, 目標, 速さ x dt) → 黄色の点が、1秒に150だけ近づく", infoX, y, MakeDemoTextStyle(20.f, yellow));

        DrawDemoLine(mode, follower, target, faint);
        DrawDemoPoint(mode, target, 16.f, red);
        DrawDemoLabel(mode, target, L"目標");
        DrawDemoPoint(mode, follower, 18.f, yellow);
    }
}
