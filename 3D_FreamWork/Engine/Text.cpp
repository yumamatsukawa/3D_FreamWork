#include "Text.h"
#include "Graphics.h"
#include <cassert>

namespace Text {

    ID2D1Factory* d2dFactory = nullptr;
    ID2D1RenderTarget* d2dRenderTarget = nullptr;
    IDWriteFactory* dwriteFactory = nullptr;

    bool Init() {
        // ★ Direct2D ファクトリ作成
        HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory);
        if (FAILED(hr)) {
            OutputDebugStringA("★ D2D1CreateFactory 失敗\n");
            return false;
        }

        // ★ DirectWrite ファクトリ作成
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&dwriteFactory));
        if (FAILED(hr)) {
            OutputDebugStringA("★ DWriteCreateFactory 失敗\n");
            return false;
        }

        // ★ DX11 の バックバッファ から D2D RenderTarget を作成
        IDXGISurface* dxgiSurface = nullptr;
        Graphics::swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface));

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        hr = d2dFactory->CreateDxgiSurfaceRenderTarget(
            dxgiSurface, &props, &d2dRenderTarget);
        dxgiSurface->Release();

        if (FAILED(hr)) {
            OutputDebugStringA("★ CreateDxgiSurfaceRenderTarget 失敗\n");
            return false;
        }

        return true;
    }

    void Uninit() {
        if (d2dRenderTarget) { d2dRenderTarget->Release(); d2dRenderTarget = nullptr; }
        if (dwriteFactory) { dwriteFactory->Release();   dwriteFactory = nullptr; }
        if (d2dFactory) { d2dFactory->Release();      d2dFactory = nullptr; }
    }

    void BeginDraw() {
        d2dRenderTarget->BeginDraw();
    }

    void EndDraw() {
        d2dRenderTarget->EndDraw();
    }

    void Draw(const std::wstring& text, float x, float y,
        float size, DirectX::XMFLOAT4 color,
        const std::wstring& fontName) {

        // ★ テキストフォーマット作成
        IDWriteTextFormat* textFormat = nullptr;
        dwriteFactory->CreateTextFormat(
            fontName.c_str(),
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size,
            L"ja-jp",
            &textFormat
        );

        // ★ テキストレイアウトを作成してサイズを計測
        IDWriteTextLayout* textLayout = nullptr;
        dwriteFactory->CreateTextLayout(
            text.c_str(),
            (UINT32)text.size(),
            textFormat,
            10000.f,  // 最大幅（大きめに設定）
            10000.f,  // 最大高さ
            &textLayout
        );

        // ★ 実際のテキストサイズを取得
        DWRITE_TEXT_METRICS metrics;
        textLayout->GetMetrics(&metrics);

        // ★ ブラシ作成
        ID2D1SolidColorBrush* brush = nullptr;
        d2dRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(color.x, color.y, color.z, color.w),
            &brush
        );

        // ★ ゲーム座標 → スクリーン座標に変換
        float screenX = x + Graphics::width / 2.f;
        float screenY = -y + Graphics::height / 2.f;

        // ★ テキストの幅・高さの半分だけずらして中心に合わせる
        screenX -= metrics.width / 2.f;
        screenY -= metrics.height / 2.f;

        D2D1_RECT_F rect = D2D1::RectF(
            screenX,
            screenY,
            screenX + metrics.width,
            screenY + metrics.height
        );

        // ★ 描画
        d2dRenderTarget->DrawText(
            text.c_str(),
            (UINT32)text.size(),
            textFormat,
            rect,
            brush
        );

        brush->Release();
        textFormat->Release();
    }
}