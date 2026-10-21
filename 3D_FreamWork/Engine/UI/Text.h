#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <DirectXMath.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// 文字の寄せ方(横)。x が文字のどこになるか
enum class TextAlign {
    Left,     // x が文字の左端
    Center,   // x が文字の中心(今までのText::Drawと同じ)
    Right     // x が文字の右端
};

// 文字の見た目。例:
//   TextStyle style;
//   style.size = 28.f;
//   style.align = TextAlign::Left;
//   style.outlineColor = { 0.f, 0.f, 0.f, 1.f };   // 黒い縁取り
//   Text::Draw(L"Score: 100", -560.f, 320.f, style);
struct TextStyle {
    float size = 32.f;                                    // 文字の大きさ(ピクセル)
    DirectX::XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f };     // 文字の色(wは濃さ。縁取りも一緒に薄くなる)
    std::wstring fontName = L"Arial";                     // フォントの名前(Text::LoadFontが返した名前か、PCにインストール済みのフォント名)
    TextAlign align = TextAlign::Center;                  // 横の寄せ方(縦はいつも中心)
    bool bold = false;                                    // 太字
    DirectX::XMFLOAT4 outlineColor = { 0.f, 0.f, 0.f, 0.f }; // 縁取りの色(a=0なら縁取り無し)
    float outlineWidth = 2.f;                             // 縁取りの太さ(ピクセル)
};

// 画面に文字を描く。座標は「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」
// Draw は Component::Draw の中(GameEngineのText::BeginDraw〜EndDrawの間)で呼ぶ。
// Scene::Draw の間に呼ばれた Draw は、その場では描かず、UIスプライトの後(オーバーレイキュー。Image.h の DeferOverlay)に呼んだ順で描かれる
// (Direct2Dは命令がたまると途中で画面に出すので、その場で描くと、後から描くUIスプライトに隠れてしまうため)。Scene::Draw の外(画面エフェクト・遷移)はその場で描く
namespace Text {
    bool Init();
    void Uninit();
    void BeginDraw();
    void EndDraw();

    // フォントファイル(.ttf/.otf)を読み込み、TextStyle::fontName に入れる名前(フォントのファミリー名)を返す。
    // 例: style.fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");   // L"Noto Sans JP" が返る
    // - 同じファイルは2回読まない(2回目からは、前に読んだ時の名前をすぐ返す)ので、何回呼んでもよい
    // - 読めなかった時は空の文字列を返し、出力ウィンドウに知らせる(空の名前で描くと、いつものフォントになる)
    // - インストールしていないフォントでも、チーム全員のPCで同じ見た目になる。フォントのライセンスファイルも一緒に置くこと
    std::wstring LoadFont(const std::wstring& path);

    // スタイルを指定して描く。(x, y) は align で決まる横の位置と、縦の中心
    void Draw(const std::wstring& text, float x, float y, const TextStyle& style);

    // 今までの書き方(中心に合わせて描く。縁取り無し・太字無し)
    void Draw(const std::wstring& text,
        float x, float y,
        float size = 32.f,
        DirectX::XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f },
        const std::wstring& fontName = L"Arial");

    // 描いた時の幅と高さ(ピクセル。縁取りの分は入らない)。Updateの中で呼んでもよい
    DirectX::XMFLOAT2 Measure(const std::wstring& text, const TextStyle& style);

    // ─── Engine内部用(ScreenEffectなどEngineの中だけで使う。ゲーム側からは呼ばない) ─────────────
    ID2D1RenderTarget* GetRenderTarget();   // 文字を描いているDirect2Dの描画先(BeginDraw〜EndDrawの間だけ描ける。Initの前はnullptr)
    // 文字を並べたレイアウトを作る(TextTexture が、画面の文字と同じフォント・並べ方でテクスチャに描くのに使う)。使い終わったらReleaseする。作れなかったらnullptr
    IDWriteTextLayout* CreateTextLayout(const std::wstring& text, const TextStyle& style, DWRITE_TEXT_METRICS& metrics);
}
