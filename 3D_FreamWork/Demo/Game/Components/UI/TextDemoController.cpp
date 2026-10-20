#include "TextDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/TextRenderer.h"
#include <cstdio>

namespace {
    constexpr float leftX = -610.f;         // 左の欄の文字の左端
    constexpr float rightX = 40.f;          // 右の欄の文字の左端
    constexpr float alignLineX = -300.f;    // 寄せ方の基準の線のX
    constexpr float typedX = 330.f;         // 入力した文字の中心のX
    constexpr float typedY = 50.f;          // 入力した文字のY

    // 文字の色に使う、薄い灰色(補足の説明用)
    constexpr DirectX::XMFLOAT4 noteColor = { 0.8f, 0.8f, 0.8f, 1.0f };
}

TextStyle TextDemoController::GetTypedStyle() const {
    // 1: ふつう / 2: 太い縁取り / 3: 太字+色 / 4: 大きい文字
    TextStyle style = MakeDemoTextStyle(36.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center);
    switch (styleIndex) {
    case 1:
        style.color = { 1.0f, 0.9f, 0.3f, 1.0f };
        style.outlineWidth = 5.f;
        break;
    case 2:
        style.size = 40.f;
        style.bold = true;
        style.color = { 0.4f, 0.95f, 1.0f, 1.0f };
        break;
    case 3:
        style.size = 56.f;
        style.color = { 1.0f, 0.6f, 0.8f, 1.0f };
        break;
    default:
        break;
    }
    return style;
}

void TextDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1〜4: 入力した文字のスタイルを選ぶ
    for (int i = 0; i < styleCount; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) styleIndex = i;
    }

    // A〜Z・SPACE: 文字を足す / BackSpace: 1文字消す
    for (int key = KEY_A; key <= KEY_Z; key++) {
        if (Input::GetKeyDown(key) && typed.size() < maxTyped) typed += (wchar_t)key;
    }
    if (Input::GetKeyDown(KEY_SPACE) && typed.size() < maxTyped) typed += L' ';
    if (Input::GetKeyDown(KEY_BACK) && !typed.empty()) typed.pop_back();

    // 動く物に付いた文字の中身を変える(TextRenderer::SetText)。物の位置が、そのまま文字になる
    wchar_t buf[48];
    swprintf_s(buf, L"x = %.0f", followerText->GetOwner()->transform.position.x);
    followerText->SetText(buf);
}

void TextDemoController::Draw() {
    // ─── 左の欄: TextStyle の項目を並べる ───

    // 大きさ(size)
    Text::Draw(L"size = 20   あいう ABC", leftX, 185.f, MakeDemoTextStyle(20.f));
    Text::Draw(L"size = 32   あいう ABC", leftX, 155.f, MakeDemoTextStyle(32.f));
    Text::Draw(L"size = 48   あいう", leftX, 108.f, MakeDemoTextStyle(48.f));

    // 色(color)
    Text::Draw(L"赤 red", leftX, 56.f, MakeDemoTextStyle(30.f, { 1.0f, 0.35f, 0.35f, 1.0f }));
    Text::Draw(L"緑 green", leftX + 160.f, 56.f, MakeDemoTextStyle(30.f, { 0.4f, 1.0f, 0.4f, 1.0f }));
    Text::Draw(L"青 blue", leftX + 350.f, 56.f, MakeDemoTextStyle(30.f, { 0.45f, 0.6f, 1.0f, 1.0f }));

    // 縁取り(outlineColor の透明度が0なら縁取り無し。太さは outlineWidth)
    TextStyle plain = MakeDemoTextStyle(28.f);
    plain.outlineColor = { 0.f, 0.f, 0.f, 0.f };
    Text::Draw(L"縁なし", leftX, 14.f, plain);
    TextStyle thin = MakeDemoTextStyle(28.f);
    Text::Draw(L"縁 2px", leftX + 130.f, 14.f, thin);
    TextStyle thick = MakeDemoTextStyle(28.f);
    thick.outlineWidth = 5.f;
    Text::Draw(L"縁 5px", leftX + 270.f, 14.f, thick);

    // 太字(bold)
    TextStyle bold = MakeDemoTextStyle(28.f);
    bold.bold = true;
    Text::Draw(L"bold = false", leftX, -26.f, MakeDemoTextStyle(28.f));
    Text::Draw(L"bold = true", leftX + 230.f, -26.f, bold);

    // 寄せ方(align)。線のXが、文字の左端 / 中心 / 右端になる
    Image::DrawRect({ alignLineX, -105.f }, { 3.f, 130.f }, { 1.0f, 0.9f, 0.4f, 1.0f });
    TextStyle alignStyle = MakeDemoTextStyle(24.f);
    alignStyle.align = TextAlign::Left;
    Text::Draw(L"Left (xが左端)", alignLineX, -70.f, alignStyle);
    alignStyle.align = TextAlign::Center;
    Text::Draw(L"Center (xが中心)", alignLineX, -105.f, alignStyle);
    alignStyle.align = TextAlign::Right;
    Text::Draw(L"Right (xが右端)", alignLineX, -140.f, alignStyle);

    // ─── 右の欄: フォントと、入力した文字 ───

    // フォント(fontName)。Text::LoadFont で読んだフォントは、インストールしていなくても使える
    TextStyle arial = MakeDemoTextStyle(24.f);
    arial.fontName = L"Arial";
    Text::Draw(L"Arial: Hello あいう", rightX, 185.f, arial);
    Text::Draw(L"Noto Sans JP (LoadFont): Hello あいう", rightX, 155.f, MakeDemoTextStyle(24.f));

    // 入力した文字: 後ろに、Text::Measure で測った大きさの板を敷く(文字より先に描く)
    Text::Draw(L"A〜Z・SPACE 入力 / BackSpace 消す / 1〜4 スタイル", rightX, 108.f, MakeDemoTextStyle(17.f, noteColor));
    const TextStyle typedStyle = GetTypedStyle();
    const std::wstring shown = typed.empty() ? L"_" : typed;
    const DirectX::XMFLOAT2 measured = Text::Measure(shown, typedStyle);
    Image::DrawRect({ typedX, typedY }, { measured.x + 24.f, measured.y + 12.f }, { 0.0f, 0.0f, 0.0f, 0.5f });
    Text::Draw(shown, typedX, typedY, typedStyle);
    wchar_t buf[96];
    swprintf_s(buf, L"スタイル %d   Text::Measure → 幅 %.0f x 高さ %.0f", styleIndex + 1, measured.x, measured.y);
    Text::Draw(buf, rightX, typedY - 52.f, MakeDemoTextStyle(18.f, { 0.7f, 1.0f, 0.7f, 1.0f }));

    // 動く物に付いた文字の説明(文字そのものは TextRenderer が描く)
    Text::Draw(L"下の物に付いた文字は TextRenderer です(物と一緒に動く)", rightX, -175.f, MakeDemoTextStyle(17.f, noteColor));
}
