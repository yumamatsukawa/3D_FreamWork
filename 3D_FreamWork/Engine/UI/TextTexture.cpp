#include "TextTexture.h"
#include "../Render/Graphics.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <vector>

using DirectX::XMFLOAT4;

namespace TextTexture {

    namespace {
        constexpr int warnCount = 256;       // 共有表の件数がこれを超えたら、出力ウィンドウに1回知らせる(止めはしない)
        constexpr float diagonal = 0.70710678f;   // 縁取りの斜めの4方向のずらし方(Text.cpp と同じ)

        // 絵を決める設定の並び替え用の鍵(floatも含めて、完全に同じ時だけ同じ鍵)
        struct Key {
            const Desc* desc;
            auto Tie() const {
                const Desc& d = *desc;
                const TextStyle& s = d.style;
                return std::tie(d.text, s.fontName, s.size, s.bold,
                    s.color.x, s.color.y, s.color.z,
                    s.outlineColor.x, s.outlineColor.y, s.outlineColor.z, s.outlineColor.w, s.outlineWidth,
                    s.align, d.background.x, d.background.y, d.background.z, d.background.w,
                    d.width, d.height);
            }
            bool operator<(const Key& other) const { return Tie() < other.Tie(); }
        };

        // 共有表。Shared は new で作り、Key は Shared 自身の desc を指す
        std::map<Key, Shared*> table;
        size_t totalBytes = 0;
        bool warned = false;

        bool HasOutline(const TextStyle& style) {
            return style.outlineColor.w > 0.f && style.outlineWidth > 0.f;
        }

        int GetMargin(const TextStyle& style, float padding) {
            const float outline = HasOutline(style) ? style.outlineWidth : 0.f;
            return (int)std::ceil((std::max)(padding, 0.f) + outline) + 2;   // +2: 端の1ピクセルは補間で隣(反対側)の色が混ざらないように空けておく
        }

        // Direct2D で文字を描いた絵(乗算済みアルファ)を、ふつうのアルファに直して返す。透明な所の色は、ミップマップで縁が黒ずまないように文字(縁取りがあれば縁取り)の色にする
        bool ReadStraight(ID3D11Texture2D* source, const Desc& desc, std::vector<unsigned char>& pixels) {
            D3D11_TEXTURE2D_DESC td{};
            source->GetDesc(&td);
            td.Usage = D3D11_USAGE_STAGING;
            td.BindFlags = 0;
            td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            td.MiscFlags = 0;
            ID3D11Texture2D* staging = nullptr;
            if (FAILED(Graphics::device->CreateTexture2D(&td, nullptr, &staging))) return false;
            Graphics::context->CopyResource(staging, source);

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (FAILED(Graphics::context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) { staging->Release(); return false; }

            const XMFLOAT4& bleed = HasOutline(desc.style) ? desc.style.outlineColor : desc.style.color;
            const unsigned char bleedB = (unsigned char)((std::min)((std::max)(bleed.z, 0.f), 1.f) * 255.f + 0.5f);
            const unsigned char bleedG = (unsigned char)((std::min)((std::max)(bleed.y, 0.f), 1.f) * 255.f + 0.5f);
            const unsigned char bleedR = (unsigned char)((std::min)((std::max)(bleed.x, 0.f), 1.f) * 255.f + 0.5f);

            pixels.resize((size_t)desc.width * desc.height * 4);
            for (int y = 0; y < desc.height; y++) {
                const unsigned char* src = (const unsigned char*)mapped.pData + (size_t)y * mapped.RowPitch;
                unsigned char* dst = &pixels[(size_t)y * desc.width * 4];
                for (int x = 0; x < desc.width; x++, src += 4, dst += 4) {
                    const unsigned a = src[3];
                    if (a == 0) { dst[0] = bleedB; dst[1] = bleedG; dst[2] = bleedR; dst[3] = 0; continue; }
                    for (int c = 0; c < 3; c++) dst[c] = (unsigned char)(std::min)(255u, ((unsigned)src[c] * 255u + a / 2u) / a);
                    dst[3] = (unsigned char)a;
                }
            }
            Graphics::context->Unmap(staging, 0);
            staging->Release();
            return true;
        }

        // 文字を Direct2D でテクスチャに描く(描いた絵は source に入る)
        bool DrawGlyphs(ID2D1Factory* factory, ID3D11Texture2D* target, const Desc& desc) {
            IDXGISurface* surface = nullptr;
            target->QueryInterface(IID_PPV_ARGS(&surface));
            if (!surface) return false;

            ID2D1RenderTarget* rt = nullptr;
            const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.f, 96.f);
            factory->CreateDxgiSurfaceRenderTarget(surface, &props, &rt);
            surface->Release();
            if (!rt) return false;

            // 文字を並べる(Text と同じフォント・同じ並べ方)。複数行の寄せ方は、いちばん長い行の幅の中で決める
            DWRITE_TEXT_METRICS metrics{};
            IDWriteTextLayout* layout = Text::CreateTextLayout(desc.text, desc.style, metrics);
            ID2D1SolidColorBrush* brush = nullptr;
            rt->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &brush);
            if (!layout || !brush) {
                if (layout) layout->Release();
                if (brush) brush->Release();
                rt->Release();
                return false;
            }
            layout->SetMaxWidth(metrics.width);
            layout->SetTextAlignment(desc.style.align == TextAlign::Left ? DWRITE_TEXT_ALIGNMENT_LEADING :
                desc.style.align == TextAlign::Right ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_CENTER);
            layout->GetMetrics(&metrics);

            rt->BeginDraw();
            rt->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));

            // 背景
            if (desc.background.w > 0.f) {
                brush->SetColor(D2D1::ColorF(desc.background.x, desc.background.y, desc.background.z, desc.background.w));
                rt->FillRectangle(D2D1::RectF(0.f, 0.f, (float)desc.width, (float)desc.height), brush);
            }

            // 文字は絵の中央に置く(レイアウトの原点は左上。文字の左上のずれを戻す)
            const D2D1_POINT_2F origin = D2D1::Point2F(
                ((float)desc.width - metrics.width) * 0.5f - metrics.left,
                ((float)desc.height - metrics.height) * 0.5f - metrics.top);

            // 縁取り: 8方向にずらして縁の色で描く(Text と同じ)
            if (HasOutline(desc.style)) {
                const XMFLOAT4& c = desc.style.outlineColor;
                brush->SetColor(D2D1::ColorF(c.x, c.y, c.z, c.w));
                const float w = desc.style.outlineWidth;
                const float d = w * diagonal;
                const D2D1_POINT_2F offsets[8] = {
                    { -w, 0.f }, { w, 0.f }, { 0.f, -w }, { 0.f, w },
                    { -d, -d }, { d, -d }, { -d, d }, { d, d },
                };
                for (const D2D1_POINT_2F& offset : offsets) {
                    rt->DrawTextLayout(D2D1::Point2F(origin.x + offset.x, origin.y + offset.y), layout, brush);
                }
            }

            // 本体(濃さはいつも1。板の色の濃さで薄くする)
            const XMFLOAT4& c = desc.style.color;
            brush->SetColor(D2D1::ColorF(c.x, c.y, c.z, 1.f));
            rt->DrawTextLayout(origin, layout, brush);

            const HRESULT hr = rt->EndDraw();
            brush->Release();
            layout->Release();
            rt->Release();
            return SUCCEEDED(hr);
        }

        // 絵を描いて、ミップマップ付きのテクスチャにする。作れなかったら nullptr
        Shared* Create(const Desc& desc) {
            if (!Graphics::device || !Graphics::context || desc.width <= 0 || desc.height <= 0) return nullptr;
            ID2D1RenderTarget* textTarget = Text::GetRenderTarget();
            if (!textTarget) return nullptr;
            ID2D1Factory* factory = nullptr;
            textTarget->GetFactory(&factory);
            if (!factory) return nullptr;

            // ① 描く用のテクスチャ(Direct2Dの描画先)
            D3D11_TEXTURE2D_DESC td{};
            td.Width = (UINT)desc.width;
            td.Height = (UINT)desc.height;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            ID3D11Texture2D* drawTexture = nullptr;
            if (FAILED(Graphics::device->CreateTexture2D(&td, nullptr, &drawTexture))) { factory->Release(); return nullptr; }

            const bool drawn = DrawGlyphs(factory, drawTexture, desc);
            factory->Release();
            std::vector<unsigned char> pixels;
            const bool read = drawn && ReadStraight(drawTexture, desc, pixels);
            drawTexture->Release();
            if (!read) return nullptr;

            // ② 表示用のテクスチャ(ミップマップ付き)。ふつうのアルファに直した絵を入れて、小さい絵を作る
            td.MipLevels = 0;
            td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
            ID3D11Texture2D* texture = nullptr;
            if (FAILED(Graphics::device->CreateTexture2D(&td, nullptr, &texture))) return nullptr;
            Graphics::context->UpdateSubresource(texture, 0, nullptr, pixels.data(), (UINT)desc.width * 4, 0);

            ID3D11ShaderResourceView* srv = nullptr;
            Graphics::device->CreateShaderResourceView(texture, nullptr, &srv);
            texture->Release();   // SRVが参照を持っている
            if (!srv) return nullptr;
            Graphics::context->GenerateMips(srv);

            Shared* shared = new Shared();
            shared->srv = srv;
            shared->width = desc.width;
            shared->height = desc.height;
            shared->bytes = (size_t)desc.width * desc.height * 4 * 4 / 3;   // ミップマップ込みのおおよその大きさ
            shared->desc = desc;
            return shared;
        }
    }

    bool Measure(const std::wstring& text, const TextStyle& style, float padding, int& width, int& height) {
        if (text.empty() || !(style.size > 0.f)) return false;
        const DirectX::XMFLOAT2 size = Text::Measure(text, style);
        if (!(size.x > 0.f) || !(size.y > 0.f)) return false;
        const int margin = GetMargin(style, padding);
        width = (int)std::ceil(size.x) + margin * 2;
        height = (int)std::ceil(size.y) + margin * 2;
        return true;
    }

    Shared* Acquire(const Desc& request) {
        Desc desc = request;
        desc.style.color.w = 1.f;   // 文字の濃さは絵に入れない
        if (!HasOutline(desc.style)) { desc.style.outlineColor = { 0.f, 0.f, 0.f, 0.f }; desc.style.outlineWidth = 0.f; }   // 縁取り無しなら、縁取りの設定の違いでは別の絵にしない

        auto it = table.find(Key{ &desc });
        if (it != table.end()) {
            it->second->refCount++;
            return it->second;
        }

        Shared* shared = Create(desc);
        if (!shared) return nullptr;
        shared->refCount = 1;
        table[Key{ &shared->desc }] = shared;
        totalBytes += shared->bytes;
        if (!warned && (int)table.size() > warnCount) {
            warned = true;
            OutputDebugStringA("★ TextTexture: 文字のテクスチャが256枚を超えました(毎フレーム変わる文字や、似た看板の多すぎが原因かもしれません)\n");
        }
        return shared;
    }

    void Release(Shared* shared) {
        if (!shared) return;
        if (--shared->refCount > 0) return;
        table.erase(Key{ &shared->desc });
        totalBytes -= shared->bytes;
        if (shared->srv) shared->srv->Release();
        delete shared;
    }

    void ReleaseAll() {
        for (auto& pair : table) {
            if (pair.second->srv) pair.second->srv->Release();
            delete pair.second;
        }
        table.clear();
        totalBytes = 0;
        warned = false;
    }

    int GetCount() { return (int)table.size(); }
    size_t GetBytes() { return totalBytes; }
}
