#include "DemoTextStyle.h"

TextStyle MakeDemoTextStyle(float size, DirectX::XMFLOAT4 color, TextAlign align) {
    // フォントの読み込み(同じファイルは2回読まないので、最初の1回だけ呼ぶ)
    static const std::wstring fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");

    TextStyle style;
    style.fontName = fontName;
    style.size = size;
    style.color = color;
    style.align = align;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    return style;
}
