#include "RectDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/Graphics.h"
#include "../../../../Engine/UI/Anchor.h"
#include <cmath>
#include <cstdio>

namespace {
    // アンカー(画面のどこを基準にするか)の9か所と、その名前
    struct AnchorLabel {
        Anchor anchor;
        const wchar_t* name;
    };
    constexpr AnchorLabel anchorLabels[] = {
        { Anchor::TopLeft, L"TopLeft" },       { Anchor::Top, L"Top" },       { Anchor::TopRight, L"TopRight" },
        { Anchor::Left, L"Left" },             { Anchor::Center, L"Center" }, { Anchor::Right, L"Right" },
        { Anchor::BottomLeft, L"BottomLeft" }, { Anchor::Bottom, L"Bottom" }, { Anchor::BottomRight, L"BottomRight" },
    };

    constexpr float marginX = 80.f;       // 左右の隅から、印までの距離
    constexpr float marginTop = 190.f;    // 上の端から、印までの距離(説明の板の下に出す)
    constexpr float marginBottom = 100.f; // 下の端から、印までの距離(戻るボタンの上に出す)
    constexpr DirectX::XMFLOAT2 anchorRectSize = { 130.f, 40.f };   // アンカーに置く四角の大きさ

    // アンカーごとの、基準からずらす量(左右は内側へ、上下も内側へ。画面座標はY+=上)
    DirectX::XMFLOAT2 GetAnchorOffset(Anchor anchor) {
        const bool left = anchor == Anchor::TopLeft || anchor == Anchor::Left || anchor == Anchor::BottomLeft;
        const bool right = anchor == Anchor::TopRight || anchor == Anchor::Right || anchor == Anchor::BottomRight;
        const bool top = anchor == Anchor::TopLeft || anchor == Anchor::Top || anchor == Anchor::TopRight;
        const bool bottom = anchor == Anchor::BottomLeft || anchor == Anchor::Bottom || anchor == Anchor::BottomRight;
        return { left ? marginX : (right ? -marginX : 0.f), top ? -marginTop : (bottom ? marginBottom : 0.f) };
    }

    constexpr float screenLimitX = 640.f;    // 四角を動かせる範囲(画面の半分の幅)
    constexpr float screenLimitY = 360.f;    // 四角を動かせる範囲(画面の半分の高さ)
}

RectDemoController::RectDemoController()
    : rects{ { { -300.f, -120.f }, { 80.f, 80.f }, { 1.0f, 0.5f, 0.5f, 0.85f } },
             { { 0.f, -120.f }, { 170.f, 90.f }, { 0.5f, 1.0f, 0.5f, 0.85f } },
             { { 300.f, -120.f }, { 270.f, 56.f }, { 0.5f, 0.7f, 1.0f, 0.85f } } } {}

int RectDemoController::FindRectAt(const DirectX::XMFLOAT2& point) const {
    // 後ろの四角ほど手前に描くので、後ろから調べる
    for (int i = dragCount - 1; i >= 0; i--) {
        if (fabsf(point.x - rects[i].center.x) <= rects[i].size.x * 0.5f && fabsf(point.y - rects[i].center.y) <= rects[i].size.y * 0.5f) return i;
    }
    return -1;
}

void RectDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // マウスの画面座標(Input::GetMousePositionはY+=下なので、Yの符号を反転する)
    const DirectX::XMFLOAT2 mouseRaw = Input::GetMousePosition();
    const DirectX::XMFLOAT2 mouse = { mouseRaw.x, -mouseRaw.y };

    // ドラッグ: 押した所にある四角をつかみ、押している間はマウスについてくる
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        dragging = FindRectAt(mouse);
        if (dragging >= 0) grabOffset = rects[dragging].center - mouse;
    }
    if (!Input::GetKeyPress(MOUSE_LEFT)) dragging = -1;
    if (dragging >= 0) {
        DragRect& rect = rects[dragging];
        rect.center = mouse + grabOffset;
        rect.center.x = Math::Clamp(rect.center.x, -screenLimitX + rect.size.x * 0.5f, screenLimitX - rect.size.x * 0.5f);
        rect.center.y = Math::Clamp(rect.center.y, -screenLimitY + rect.size.y * 0.5f, screenLimitY - rect.size.y * 0.5f);
    }
}

void RectDemoController::Draw() {
    // アンカーごとに、印(黄色の小さな四角=GetAnchoredPosition)と、印にかどを合わせた四角(青=GetAnchoredCenter)
    for (const AnchorLabel& label : anchorLabels) {
        const DirectX::XMFLOAT2 offset = GetAnchorOffset(label.anchor);
        const DirectX::XMFLOAT2 center = GetAnchoredCenter(label.anchor, offset, anchorRectSize);
        Image::DrawRect(center, anchorRectSize, { 0.2f, 0.4f, 0.8f, 0.55f });

        const DirectX::XMFLOAT2 point = GetAnchoredPosition(label.anchor, offset);
        Image::DrawRect(point, { 12.f, 12.f }, { 1.0f, 0.9f, 0.4f, 1.0f });
        Text::Draw(label.name, center.x, center.y, MakeDemoTextStyle(18.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    }

    // 画面の中央の説明(Graphics::width / height は、今のゲーム画面の大きさ)
    wchar_t buf[128];
    swprintf_s(buf, L"画面の大きさ  %.0f x %.0f  (Graphics::width / height)", Graphics::width, Graphics::height);
    Text::Draw(L"黄色の印 = GetAnchoredPosition(画面の9か所の位置)", 0.f, 112.f, MakeDemoTextStyle(19.f, { 1.0f, 0.9f, 0.4f, 1.0f }, TextAlign::Center));
    Text::Draw(L"青い四角 = GetAnchoredCenter(印に四角の角や辺を合わせる)", 0.f, 86.f, MakeDemoTextStyle(19.f, { 0.6f, 0.8f, 1.0f, 1.0f }, TextAlign::Center));
    Text::Draw(buf, 0.f, 60.f, MakeDemoTextStyle(19.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));

    // ドラッグできる四角(Image::DrawRect)。つかんでいる間は枠を白くして、位置と大きさを下に出す
    for (int i = 0; i < dragCount; i++) {
        const DragRect& rect = rects[i];
        if (i == dragging) Image::DrawRect(rect.center, { rect.size.x + 6.f, rect.size.y + 6.f }, { 1.0f, 1.0f, 1.0f, 1.0f });
        Image::DrawRect(rect.center, rect.size, rect.color);
        swprintf_s(buf, L"%.0f x %.0f", rect.size.x, rect.size.y);
        Text::Draw(buf, rect.center.x, rect.center.y, MakeDemoTextStyle(17.f, { 0.0f, 0.0f, 0.0f, 1.0f }, TextAlign::Center));
        swprintf_s(buf, L"(%.0f, %.0f)", rect.center.x, rect.center.y);
        Text::Draw(buf, rect.center.x, rect.center.y - rect.size.y * 0.5f - 16.f, MakeDemoTextStyle(17.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    }
}
