#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <DirectXMath.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Text {
    bool Init();
    void Uninit();
    void BeginDraw();
    void EndDraw();

    // テキスト描画
    void Draw(const std::wstring& text,
        float x, float y,
        float size = 32.f,
        DirectX::XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f },
        const std::wstring& fontName = L"Arial");
}