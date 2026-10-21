#pragma once
#include "../Core/Component.h"
#include "../UI/Text.h"
#include "../UI/TextTexture.h"
#include "Mesh.h"
#include "Lod.h"
#include <DirectXMath.h>
#include <memory>
#include <string>

// 3Dの世界に貼り付ける文字(看板の文字)を描くComponent。TextRenderer(画面の文字)と違い、文字は3Dの板になる。
// 壁に貼って傾けられ、物の後ろに隠れ、カメラから遠いほど小さく見える。
//
// 使い方:
//   auto* sign = obj->AddComponent<WorldTextRenderer>();
//   sign->SetText(L"WELCOME");
//   TextStyle style; style.size = 64.f; style.outlineColor = { 0.f, 0.f, 0.f, 1.f };
//   sign->SetStyle(style);
//   obj->transform.rotate = { 0.f, 30.f, 0.f };   // 板の向き(オーナーの回転)
//   sign->SetBillboard(true);                     // いつもカメラの方を向く
//
// 決まり:
// - 文字は Direct2D でテクスチャに描き、四角い板(Mesh)に貼って、MeshRenderer と同じ経路(Shader3D・深度バッファ)で描く
// - style.size は「テクスチャ上のピクセル」。板のワールドでの大きさは「テクスチャのピクセル数 ÷ SetPixelsPerUnit」(既定 2。文字の高さ64pxなら32ワールド)。
//   さらにオーナーの transform.scale で拡大縮小できる。テクスチャは2048ピクセルまで(超える時は、見た目の大きさを変えずに細かさを下げる)
// - 板の正面は -Z 側(カメラが +Z を向く既定の向きで、文字が普通に読める。SpriteRenderer のビルボードと同じ)。位置・向き・大きさは、オーナーの transform(親子込みのワールド行列)
// - align: Center は板の中心がオーナーの位置、Left は板の左端、Right は板の右端がオーナーの位置
// - ビルボード(SetBillboard(true)): オーナーの回転は使わず、いつもカメラ(CameraShake込み)の方を向く。SetBillboardLock で軸ごとに止められる(SpriteRenderer と同じ。trueにした軸はカメラに合わせて回らない)。transform.rotate.z だけは板の中の回転として使う
// - 裏側から見た時: SetDoubleSided(true)(既定)なら、裏でも文字が読める向きで見える。falseなら見えない
// - アルファ: 文字のふちの半透明を保ったまま、深度に書かず(物の後ろに隠れるが、後ろの物を隠さない)、半透明で重ねる。Init で、オーナーの drawPriority が 0 なら 10 にして
//   (壁より後に描く)、板の後ろの壁に隠されないようにする。変えたい時は GetOwner()->SetDrawPriority()。背景が不透明(SetBackground の a=1・文字の濃さ1)なら、深度にも書く(ふつうの板)
// - 同じ絵のテクスチャは共有する(TextTexture.h): 文字・フォント・ピクセルの大きさ・太字・色・縁取り・寄せ方・背景・テクスチャの大きさが全部同じなら、板が何枚あってもテクスチャは1枚。
//   文字や見た目を変えると、今のテクスチャを手放して、新しい絵のテクスチャに切り替える(ほかの看板のテクスチャは書き換えない)。借りている数が0になったテクスチャは、すぐ解放される。
//   毎フレーム変わる文字(カウントなど)は共有されず、そのたびに新しいテクスチャを作る(重い。数字が変わる時だけ SetText する)
// - 光(SetLit)は既定で受けない(読みやすい)。影は落とさない・受けない
class WorldTextRenderer : public Component {
private:
    static constexpr int maxTextureSize = 2048;        // テクスチャの大きさの上限(ピクセル)
    static constexpr int defaultDrawPriority = 10;     // オーナーの drawPriority が0の時に付ける値(壁などの不透明な物より後に描く)

    std::wstring text;                                 // 表示する文字
    TextStyle style;                                   // 見た目(size はテクスチャ上のピクセル)
    float pixelsPerUnit = 2.f;                         // 1ワールド単位あたりのテクスチャのピクセル数
    DirectX::XMFLOAT4 background{ 0.f, 0.f, 0.f, 0.f };  // 背景の板の色(a=0なら背景なし)
    float padding = 0.f;                               // 背景の余白(ピクセル)
    bool billboard = false;                            // trueならいつもカメラの方を向く
    bool lockX = false;                                // ビルボードで止める軸(trueにした軸は回らない)
    bool lockY = false;
    bool lockZ = false;
    bool doubleSided = true;                           // 裏側からも見えるか
    bool lit = false;                                  // 光を受けるか
    bool frustumCulling = false;                       // trueなら、画面の外にある時は描かない
    LodSettings lod;                                   // 距離で省く設定(drawDistance だけが効く)

    bool dirty = true;                                 // 文字や見た目が変わった(次に描く時にテクスチャを借り直す)
    TextTexture::Shared* texture = nullptr;            // 今借りている共有テクスチャ
    float shrink = 1.f;                                // 2048を超えて細かさを下げた割合(1=下げていない)
    std::shared_ptr<Mesh> mesh;                        // 板(-0.5〜0.5の正方形。大きさは描く時の行列で決める)
    bool meshReady = false;                            // meshを作った(Uninitで1回だけ解放するため)

    void Rebuild();                                    // 今の設定のテクスチャを借り直す
    void ReleaseTexture();                             // 借りているテクスチャを返す

public:
    ~WorldTextRenderer() override;

    void SetText(const std::wstring& value);                  // 表示する文字
    const std::wstring& GetText() const { return text; }
    void SetStyle(const TextStyle& value);                    // 見た目(大きさ・色・フォント・縁取り・寄せ方)
    const TextStyle& GetStyle() const { return style; }
    void SetColor(DirectX::XMFLOAT4 color);                   // 文字の色だけ変える(wは板全体の濃さ。wだけ変える時は絵を作り直さない)
    void SetPixelsPerUnit(float value);                       // 1ワールド単位あたりのピクセル数(大きいほど板が小さくなる。0以下は無視)
    float GetPixelsPerUnit() const { return pixelsPerUnit; }
    // 背景の板の色。a=0で背景なし。paddingPixels は文字の周りの余白(ピクセル)。看板の板に使う
    void SetBackground(DirectX::XMFLOAT4 color, float paddingPixels = 0.f);

    void SetBillboard(bool value) { billboard = value; }      // true=いつもカメラの方を向く / false=オーナーの回転のまま(既定)
    bool IsBillboard() const { return billboard; }
    // ビルボードで止める軸。例: SetBillboardLock(true, false, true) で、横(Y軸)だけカメラに向く立て看板
    void SetBillboardLock(bool x, bool y, bool z) { lockX = x; lockY = y; lockZ = z; }
    void SetDoubleSided(bool value) { doubleSided = value; }  // 裏側からも見えるか(既定true)
    bool IsDoubleSided() const { return doubleSided; }
    void SetLit(bool value) { lit = value; }                  // 光を受けるか(既定false)
    bool IsLit() const { return lit; }

    // trueにすると、カメラに映らない時は描かない(既定false)
    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }
    // 距離で省く設定(drawDistance。0=いつも描く)
    void SetLod(const LodSettings& settings) { lod = settings; }
    LodSettings& GetLod() { return lod; }
    const LodSettings& GetLod() const { return lod; }

    // 今借りているテクスチャの大きさ(ピクセル)と、板のワールドでの大きさ(Updateの後・描いた後に取れる。まだなら0)
    int GetTextureWidth() const { return texture ? texture->width : 0; }
    int GetTextureHeight() const { return texture ? texture->height : 0; }
    DirectX::XMFLOAT2 GetWorldSize() const;
    // 借りているテクスチャ(同じ絵なら、ほかの看板と同じ物が返る。共有の確認用)
    const TextTexture::Shared* GetTexture() const { return texture; }

    // デバッグ用: 共有されている文字のテクスチャの数と、おおよその合計のメモリ(バイト)
    static int GetSharedTextureCount() { return TextTexture::GetCount(); }
    static size_t GetSharedTextureBytes() { return TextTexture::GetBytes(); }

    void Init() override;
    void Draw() override;
    void Uninit() override;
};
