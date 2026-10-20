#include "DemoMarker.h"
#include "DemoTextStyle.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include <cmath>

bool WorldToDemoScreen(DemoMode mode, const DirectX::XMFLOAT3& position, DirectX::XMFLOAT2& screen) {
    // 3Dはカメラの後ろ(z <= 0)にある点は出さない
    if (mode == DemoMode::Mode3D) {
        const DirectX::XMFLOAT3 projected = Image::WorldToScreen3D(position);
        screen = { projected.x, projected.y };
        return projected.z > 0.f;
    }
    screen = Image::WorldToScreen2D(position);
    return true;
}

void DrawDemoPoint(DemoMode mode, const DirectX::XMFLOAT3& position, float size, const DirectX::XMFLOAT4& color) {
    DirectX::XMFLOAT2 screen;
    if (WorldToDemoScreen(mode, position, screen)) Image::DrawRect(screen, { size, size }, color);
}

void DrawDemoLine(DemoMode mode, const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT4& color, int dotCount) {
    // 点を並べて線にする。先の点は大きくして矢印の先にする
    if (dotCount < 2) dotCount = 2;
    for (int i = 0; i < dotCount; i++) {
        DrawDemoPoint(mode, Math::Lerp(from, to, (float)i / (float)dotCount), 4.f, color);
    }
    DrawDemoPoint(mode, to, 11.f, color);
}

void DrawDemoLabel(DemoMode mode, const DirectX::XMFLOAT3& position, const std::wstring& text, float yOffset) {
    DirectX::XMFLOAT2 screen;
    if (WorldToDemoScreen(mode, position, screen)) {
        Text::Draw(text, screen.x, screen.y + yOffset, MakeDemoTextStyle(18.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    }
}

void DrawDemoRing(DemoMode mode, const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT4& color, int dotCount) {
    // 円周を点で並べる(2DはXY、3Dは水平のXZ)
    for (int i = 0; i < dotCount; i++) {
        const float angle = 6.2832f * (float)i / (float)dotCount;
        const float x = cosf(angle) * radius, y = sinf(angle) * radius;
        const DirectX::XMFLOAT3 p = (mode == DemoMode::Mode2D) ? DirectX::XMFLOAT3{ center.x + x, center.y + y, center.z }
                                                              : DirectX::XMFLOAT3{ center.x + x, center.y, center.z + y };
        DrawDemoPoint(mode, p, 4.f, color);
    }
}

void DrawDemoRect(DemoMode mode, const DirectX::XMFLOAT3& center, DirectX::XMFLOAT2 size, const DirectX::XMFLOAT4& color) {
    // 平面の4つの角(2DはXY、3Dは水平のXZ)を、順に線でつなぐ
    const float hw = size.x * 0.5f, hh = size.y * 0.5f;
    const float cornerX[4] = { -hw, hw, hw, -hw };
    const float cornerY[4] = { -hh, -hh, hh, hh };
    DirectX::XMFLOAT3 corners[4];
    for (int i = 0; i < 4; i++) {
        corners[i] = (mode == DemoMode::Mode2D) ? DirectX::XMFLOAT3{ center.x + cornerX[i], center.y + cornerY[i], center.z }
                                                : DirectX::XMFLOAT3{ center.x + cornerX[i], center.y, center.z + cornerY[i] };
    }
    for (int i = 0; i < 4; i++) {
        const DirectX::XMFLOAT3& from = corners[i];
        const DirectX::XMFLOAT3& to = corners[(i + 1) % 4];
        DrawDemoLine(mode, from, to, color, (int)Math::Clamp(Math::Distance(from, to) / 12.f, 4.f, 40.f));
    }
}
