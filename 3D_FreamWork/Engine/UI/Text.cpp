#include "Text.h"
#include "../Render/Graphics.h"
#include "../Render/Image.h"
#include <dwrite_3.h>
#include <cassert>
#include <cwchar>
#include <map>
#include <tuple>
#include <vector>

using DirectX::XMFLOAT2;
using DirectX::XMFLOAT4;

namespace Text {

    ID2D1Factory* d2dFactory = nullptr;
    ID2D1RenderTarget* d2dRenderTarget = nullptr;
    IDWriteFactory* dwriteFactory = nullptr;

    namespace {
        constexpr size_t maxFormatCount = 64;   // 取っておくフォント設定の上限(超えたら全部捨てて作り直す。大きさを毎フレーム変えても増え続けないように)
        constexpr float layoutMaxSize = 10000.f; // 文字を並べる枠の大きさ(大きめにして、はみ出さないようにする)
        constexpr float diagonal = 0.70710678f;  // 縁取りの斜めの4方向のずらし方(斜めでも縦横と同じ距離になるように1/√2を掛ける)

        // 描く時に使い回す物(最初に要った時に作る。Uninitで解放)
        ID2D1SolidColorBrush* brush = nullptr;   // 文字と縁取りのブラシ(色はSetColorで変える)
        ID2D1Layer* fadeLayer = nullptr;         // 縁取り付きの文字を薄くする時に使う重ね描きの層

        // LoadFontで読んだフォント
        struct LoadedFont {
            std::wstring fullPath;     // 絶対パス(同じファイルを2回読まないため)
            std::wstring familyName;   // 使う時の名前(読めなかったファイルは空)
        };
        std::vector<LoadedFont> loadedFonts;            // 読んだ(読もうとした)ファイル
        IDWriteFactory5* dwriteFactory5 = nullptr;      // フォントファイルを読むのに使う(Windows 10以降。使えなければnullptr)
        std::vector<IDWriteFontSet*> fontSets;          // 読めたフォントファイル1つずつのフォントの集まり
        IDWriteFontCollection1* fontCollection = nullptr; // 読めたフォント全部の一覧(CreateTextFormatに渡す)

        // フォント設定(IDWriteTextFormat)は「フォント名・大きさ・太字」ごとに作って取っておく
        using FormatKey = std::tuple<std::wstring, float, bool>;
        std::map<FormatKey, IDWriteTextFormat*> formatCache;

        void ReleaseFormats() {
            for (auto& pair : formatCache) pair.second->Release();
            formatCache.clear();
        }

        // フォント設定を取ってくる(無ければ作る)。作れなかったらnullptr
        IDWriteTextFormat* GetFormat(const TextStyle& style) {
            if (!dwriteFactory || !(style.size > 0.f)) return nullptr;   // 大きさが0以下(やNaN)は作れない

            // 名前が空(LoadFontに失敗した時など)なら、いつものフォント
            const std::wstring& fontName = style.fontName.empty() ? TextStyle().fontName : style.fontName;
            FormatKey key(fontName, style.size, style.bold);
            auto it = formatCache.find(key);
            if (it != formatCache.end()) return it->second;

            if (formatCache.size() >= maxFormatCount) ReleaseFormats();

            // LoadFontで読んだフォントの名前なら、その一覧から探す。それ以外はPCにインストール済みのフォントから
            IDWriteFontCollection* collection = nullptr;
            if (fontCollection) {
                UINT32 index = 0;
                BOOL exists = FALSE;
                if (SUCCEEDED(fontCollection->FindFamilyName(fontName.c_str(), &index, &exists)) && exists) {
                    collection = fontCollection;
                }
            }

            IDWriteTextFormat* format = nullptr;
            dwriteFactory->CreateTextFormat(
                fontName.c_str(),
                collection,
                style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                style.size,
                L"ja-jp",
                &format
            );
            if (!format) return nullptr;
            formatCache[key] = format;
            return format;
        }

        // 文字を並べて(レイアウトして)大きさを測る。使い終わったらReleaseする。作れなかったらnullptr
        IDWriteTextLayout* CreateLayout(const std::wstring& text, const TextStyle& style, DWRITE_TEXT_METRICS& metrics) {
            IDWriteTextFormat* format = GetFormat(style);
            if (!format) return nullptr;

            IDWriteTextLayout* layout = nullptr;
            dwriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.size(), format, layoutMaxSize, layoutMaxSize, &layout);
            if (!layout) return nullptr;

            // 折り返さない(改行文字の所だけ改行する)
            layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            layout->GetMetrics(&metrics);
            return layout;
        }

        bool PrepareBrush() {
            if (brush) return true;
            d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &brush);
            return brush != nullptr;
        }

        bool PrepareLayer() {
            if (fadeLayer) return true;
            d2dRenderTarget->CreateLayer(nullptr, &fadeLayer);
            return fadeLayer != nullptr;
        }

        // フォントの一覧の、index番目のファミリーの名前(英語の名前があればそれ、無ければ最初の名前)
        std::wstring GetFamilyName(IDWriteFontCollection* collection, UINT32 index) {
            IDWriteFontFamily* family = nullptr;
            collection->GetFontFamily(index, &family);
            if (!family) return L"";

            std::wstring name;
            IDWriteLocalizedStrings* names = nullptr;
            family->GetFamilyNames(&names);
            if (names) {
                UINT32 nameIndex = 0;
                BOOL exists = FALSE;
                names->FindLocaleName(L"en-us", &nameIndex, &exists);
                if (!exists) nameIndex = 0;
                UINT32 length = 0;
                if (SUCCEEDED(names->GetStringLength(nameIndex, &length))) {
                    std::vector<wchar_t> buffer(length + 1);
                    if (SUCCEEDED(names->GetString(nameIndex, buffer.data(), length + 1))) name = buffer.data();
                }
                names->Release();
            }
            family->Release();
            return name;
        }

        // 読めたフォント全部(fontSets)から、フォントの一覧を作り直す
        bool RebuildCollection() {
            IDWriteFontSetBuilder1* builder = nullptr;
            dwriteFactory5->CreateFontSetBuilder(&builder);
            if (!builder) return false;
            for (IDWriteFontSet* set : fontSets) builder->AddFontSet(set);

            IDWriteFontSet* allSet = nullptr;
            builder->CreateFontSet(&allSet);
            builder->Release();
            if (!allSet) return false;

            IDWriteFontCollection1* collection = nullptr;
            dwriteFactory5->CreateFontCollectionFromFontSet(allSet, &collection);
            allSet->Release();
            if (!collection) return false;

            if (fontCollection) fontCollection->Release();
            fontCollection = collection;
            ReleaseFormats();   // 前の一覧で作ったフォント設定は、新しく読んだフォントを知らないので作り直す
            return true;
        }

        // 1つのフォントファイルを読み、そのファイルだけのフォントの集まりとファミリー名を返す
        IDWriteFontSet* ReadFontFile(const std::wstring& fullPath, std::wstring& familyName) {
            IDWriteFontSetBuilder1* builder = nullptr;
            dwriteFactory5->CreateFontSetBuilder(&builder);
            if (!builder) return nullptr;

            IDWriteFontFile* file = nullptr;
            dwriteFactory5->CreateFontFileReference(fullPath.c_str(), nullptr, &file);
            if (!file) { builder->Release(); return nullptr; }
            HRESULT hr = builder->AddFontFile(file);   // フォントでないファイルなら失敗する
            file->Release();

            IDWriteFontSet* set = nullptr;
            if (SUCCEEDED(hr)) builder->CreateFontSet(&set);
            builder->Release();
            if (!set) return nullptr;
            if (set->GetFontCount() == 0) { set->Release(); return nullptr; }

            // ファミリー名は、このファイルだけで一覧を作って調べる
            IDWriteFontCollection1* collection = nullptr;
            dwriteFactory5->CreateFontCollectionFromFontSet(set, &collection);
            if (collection) {
                if (collection->GetFontFamilyCount() > 0) familyName = GetFamilyName(collection, 0);
                collection->Release();
            }
            if (familyName.empty()) { set->Release(); return nullptr; }
            return set;
        }

        void ReportLoadFailed(const std::wstring& path, const wchar_t* reason) {
            std::wstring message = L"★ Text::LoadFont: フォントを読み込めませんでした(" + std::wstring(reason) + L"): " + path + L"\n";
            OutputDebugStringW(message.c_str());
        }
    }

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
        // 使い回していた物(描画先・ファクトリより先に)
        ReleaseFormats();
        if (brush) { brush->Release(); brush = nullptr; }
        if (fadeLayer) { fadeLayer->Release(); fadeLayer = nullptr; }

        // 読み込んだフォント
        if (fontCollection) { fontCollection->Release(); fontCollection = nullptr; }
        for (IDWriteFontSet* set : fontSets) set->Release();
        fontSets.clear();
        loadedFonts.clear();
        if (dwriteFactory5) { dwriteFactory5->Release(); dwriteFactory5 = nullptr; }

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

    ID2D1RenderTarget* GetRenderTarget() {
        return d2dRenderTarget;
    }

    std::wstring LoadFont(const std::wstring& path) {
        if (!dwriteFactory) {
            ReportLoadFailed(path, L"Text::Initの前");
            return L"";
        }

        // 絶対パスに直す(DirectWriteは絶対パスでしか読めない。同じファイルかどうかもこれで比べる)
        DWORD length = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
        if (length == 0) {
            ReportLoadFailed(path, L"パスが正しくない");
            return L"";
        }
        std::vector<wchar_t> buffer(length);
        if (GetFullPathNameW(path.c_str(), length, buffer.data(), nullptr) == 0) {
            ReportLoadFailed(path, L"パスが正しくない");
            return L"";
        }
        std::wstring fullPath = buffer.data();

        // 同じファイルは2回読まない(読めなかったファイルも、2回目からはすぐ空を返す。知らせるのも1回だけ)
        for (const LoadedFont& font : loadedFonts) {
            if (_wcsicmp(font.fullPath.c_str(), fullPath.c_str()) == 0) return font.familyName;
        }
        const size_t entry = loadedFonts.size();   // 読めたら、ここに名前を入れる
        loadedFonts.push_back({ fullPath, L"" });

        // フォントファイルを読む仕組み(IDWriteFactory5)は Windows 10 以降にしか無い
        if (!dwriteFactory5) {
            dwriteFactory->QueryInterface(__uuidof(IDWriteFactory5), reinterpret_cast<void**>(&dwriteFactory5));
            if (!dwriteFactory5) {
                ReportLoadFailed(path, L"このWindowsでは使えない。Windows 10以降が必要");
                return L"";
            }
        }

        // 読んで、読めたフォント全部の一覧を作り直す
        std::wstring familyName;
        IDWriteFontSet* set = ReadFontFile(fullPath, familyName);
        if (!set) {
            ReportLoadFailed(path, L"ファイルが無い、またはフォントのファイルではない");
            return L"";
        }
        fontSets.push_back(set);
        if (!RebuildCollection()) {
            fontSets.pop_back();
            set->Release();
            ReportLoadFailed(path, L"フォントの一覧を作れない");
            return L"";
        }

        loadedFonts[entry].familyName = familyName;   // 2回目からはこの名前を返す
        return familyName;
    }

    IDWriteTextLayout* CreateTextLayout(const std::wstring& text, const TextStyle& style, DWRITE_TEXT_METRICS& metrics) {
        return CreateLayout(text, style, metrics);
    }

    XMFLOAT2 Measure(const std::wstring& text, const TextStyle& style) {
        DWRITE_TEXT_METRICS metrics{};
        IDWriteTextLayout* layout = CreateLayout(text, style, metrics);
        if (!layout) return { 0.f, 0.f };
        layout->Release();
        return { metrics.width, metrics.height };
    }

    namespace {
    // 文字を今すぐ描く(Direct2D)。Scene::Drawの間は、Drawが積んでおいて、UIスプライトの後にこれを呼ぶ
    void DrawNow(const std::wstring& text, float x, float y, const TextStyle& style) {
        if (!d2dRenderTarget || text.empty()) return;
        const float alpha = style.color.w;
        if (alpha <= 0.f) return;   // 透明なら描かない

        DWRITE_TEXT_METRICS metrics{};
        IDWriteTextLayout* layout = CreateLayout(text, style, metrics);
        if (!layout) return;   // 作れなかった時は描かない
        if (!PrepareBrush()) { layout->Release(); return; }

        // ★ ゲーム座標 → スクリーン座標に変換
        float screenX = x + Graphics::width / 2.f;
        float screenY = -y + Graphics::height / 2.f;

        // ★ 横は align に合わせてずらす(Left: xが左端、Center: xが中心、Right: xが右端)。縦はいつも中心
        if (style.align == TextAlign::Center) screenX -= metrics.width / 2.f;
        else if (style.align == TextAlign::Right) screenX -= metrics.width;
        screenY -= metrics.height / 2.f;

        // ★ 測ったレイアウトをそのまま描く
        // (DrawTextに幅ぴったりの四角を渡すと、レイアウトし直した時の小数の誤差で最後の文字が折り返されるため)
        // レイアウトの原点は左上。文字の左上(metrics.left/top)が原点からずれている分を戻す
        const D2D1_POINT_2F origin = D2D1::Point2F(screenX - metrics.left, screenY - metrics.top);

        // 縁取り付きの文字を薄くする時は、濃いまま層(レイヤー)に描いてから、層ごと薄くする
        // (そのまま薄く描くと、本体の下の縁取りが透けて、文字が黒っぽく見えるため)
        const bool hasOutline = style.outlineColor.w > 0.f && style.outlineWidth > 0.f;
        const bool useLayer = hasOutline && alpha < 1.f && PrepareLayer();
        const float drawAlpha = useLayer ? 1.f : alpha;
        if (useLayer) {
            // 層の範囲は文字の周り(縁取りと、字が枠から少しはみ出す分の余裕を足す)
            const float margin = style.outlineWidth + style.size * 0.5f;
            const D2D1_RECT_F bounds = D2D1::RectF(screenX - margin, screenY - margin,
                screenX + metrics.width + margin, screenY + metrics.height + margin);
            d2dRenderTarget->PushLayer(
                D2D1::LayerParameters(bounds, nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), alpha),
                fadeLayer);
        }

        // 縁取り: 8方向に outlineWidth ずらして縁の色で描く(本体の濃さも掛ける)
        if (hasOutline) {
            const XMFLOAT4& c = style.outlineColor;
            brush->SetColor(D2D1::ColorF(c.x, c.y, c.z, c.w * drawAlpha));
            const float w = style.outlineWidth;
            const float d = w * diagonal;
            const D2D1_POINT_2F offsets[8] = {
                { -w, 0.f }, { w, 0.f }, { 0.f, -w }, { 0.f, w },
                { -d, -d }, { d, -d }, { -d, d }, { d, d },
            };
            for (const D2D1_POINT_2F& offset : offsets) {
                d2dRenderTarget->DrawTextLayout(D2D1::Point2F(origin.x + offset.x, origin.y + offset.y), layout, brush);
            }
        }

        // 本体
        const XMFLOAT4& c = style.color;
        brush->SetColor(D2D1::ColorF(c.x, c.y, c.z, drawAlpha));
        d2dRenderTarget->DrawTextLayout(origin, layout, brush);

        if (useLayer) d2dRenderTarget->PopLayer();
        layout->Release();
    }
    }   // namespace

    void Draw(const std::wstring& text, float x, float y, const TextStyle& style) {
        // Scene::Drawの間は積んでおく(文字列・位置・スタイルは値のコピー)。それ以外(画面エフェクト・遷移など)はその場で描く
        if (Image::DeferOverlay([text, x, y, style] { DrawNow(text, x, y, style); })) return;
        DrawNow(text, x, y, style);
    }

    void Draw(const std::wstring& text, float x, float y,
        float size, DirectX::XMFLOAT4 color,
        const std::wstring& fontName) {
        // 今までの書き方: 中心に合わせて、縁取り無し・太字無しで描く
        TextStyle style;
        style.size = size;
        style.color = color;
        style.fontName = fontName;
        Draw(text, x, y, style);
    }
}
