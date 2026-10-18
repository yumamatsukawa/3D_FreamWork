#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "../Render/Camera.h"
#include "Light.h"

class Component;

using namespace DirectX;

// 影(シャドウマップ)。光の位置・向きから見た「深度」を専用のテクスチャに描き、
// 本番の描画で「光から見て手前に別の物があるピクセル」を暗くする。
// 使い方: Shadow::GetSettings().enabled = true; にして、影を落としたい物の Material に castShadow = true、
// 影を受けたい物(地面など)に receiveShadow = true を指定する(どちらも既定はfalse)。
//
// 決まり:
// - 既定は無効。無効の間は影のパスを実行しない(コスト0)し、見た目も影が無かった頃と同じ
// - 影が出る光は、平行光源(Image::GetLight())と、LightComponent::SetCastShadow(true) にしたスポットライト1つだけ。
//   環境光とポイントライト(点光源)は影で暗くならない(ポイントライトの影は作れない)。スポットライトの影は、その光だけを暗くする
// - 影を落とせるのは MeshRenderer / ModelRenderer(スキンメッシュを含む)。SpriteRenderer(画像)は落とせない
// - 影の範囲は、3Dカメラの前方 distance の箱。範囲の外では影が出ない
namespace Shadow {
    // 影の設定(1つだけ)
    struct Settings {
        bool  enabled = false;       // 影を使うか。既定はOFF(今の見た目のまま)
        int   resolution = 2048;     // 影のテクスチャの大きさ(正方形。1024 / 2048 / 4096)。大きいほどくっきりして重い
        float distance = 3000.f;     // カメラの前方、この大きさの範囲に影を作る。広いほど影がぼやける
        float depthBias = 0.0005f;   // 影の判定のゆとり(0〜1の深度)。小さすぎると縞模様(シャドウアクネ)が出て、大きすぎると影が物から浮く
        float normalBias = 2.0f;     // 面の法線の方向へずらして判定する量(ワールド単位)。縞模様が消えない時に増やす
        float strength = 0.6f;       // 影の濃さ(0=影なし、1=平行光源の光が真っ暗)
        int   pcfRadius = 1;         // 影の縁のぼかし(0=ぼかさない、1=3x3、2=5x5)

        // ── スポットライトの影(LightComponent::SetCastShadow(true) の、3Dカメラに一番近いスポット1つ) ──
        int   spotResolution = 1024;   // スポットライトの影のテクスチャの大きさ(正方形)
        float spotDepthBias = 3.0f;    // スポットの影の判定のゆとり(ワールド単位。光から見た奥行き方向)
        float spotStrength = 1.0f;     // スポットの影の濃さ(0=影なし、1=そのスポットの光が真っ暗)。normalBias と pcfRadius は平行光源と同じ値を使う
    };
    void SetSettings(const Settings& settings);
    Settings& GetSettings();

    // 光から見た行列(影のパスで、影を落とす物を描く時に渡す)
    struct LightView {
        XMMATRIX view;   // ワールド → 光から見た空間
        XMMATRIX proj;   // 光から見た空間 → 画面(平行光源なので正射影)
    };

    // 影を落とす物(MeshRenderer / ModelRenderer)が実装する。影のパスで1回ずつ呼ばれ、深度だけを描く
    class Caster {
    public:
        virtual ~Caster() = default;
        virtual void DrawShadow(const LightView& light) = 0;
    };

    // 登録・解除(MeshRenderer / ModelRenderer の Init / Uninit / デストラクタが呼ぶ)。
    // component は「アクティブ(SetActive)で有効(SetEnabled)か」を調べるのに使う
    void Register(Component* component, Caster* caster);
    void Unregister(Caster* caster);

    // スポットライトから見る行列を作る(透視投影。視野は外側の角度の2倍で、最大120度。奥行きは光の届く距離 range まで)。Render が使う(確認用に公開)
    LightView BuildSpotView(XMFLOAT3 position, XMFLOAT3 direction, float outerAngleDegrees, float range);

    // カメラの前方 distance の範囲を、光の向きから見る行列を作る。
    // カメラが動いても影の縁がちらつかないよう、光から見た位置をテクセルの大きさの整数倍に丸める。Render が使う(確認用に公開)
    LightView BuildSunView(const Camera& camera, const Light& light, float distance, int resolution);

    // ─── Engine内部用(GameEngine と Mesh / SkinMesh だけが呼ぶ) ───
    void Render();     // 影のパス: 深度テクスチャに「影を落とす物」を描く → 描画先・ビューポート・各ステートを元に戻す。無効の時は何もしない
    void Bind();       // Mesh / SkinMesh が描く時に、影の定数バッファ(b5)・深度テクスチャ(t1 平行光源 / t2 スポット)・比較サンプラー(s1)をセットする
    void Shutdown();   // 終了時に、テクスチャ・ステートと登録を片付ける

    // シェーダー(Shader3DCommon.hlsli の ShadowBuffer, b5)に送る形。16バイトずつ区切って並べる
    struct ShadowConstantBuffer {
        XMMATRIX viewProj;   // 光から見た view × proj(転置して送る)
        XMFLOAT4 params;     // x=影を使う(1=ON), y=depthBias, z=normalBias, w=strength
        XMFLOAT4 params2;    // x=1/解像度, y=ぼかしの半径(pcfRadius)
        XMMATRIX spotViewProj;   // スポットライトから見た view × proj(転置して送る)
        XMFLOAT4 spotParams;     // x=スポットの影を使う(1=ON), y=depthBias(ワールド単位), z=near, w=far
        XMFLOAT4 spotParams2;    // x=1/解像度, y=ぼかしの半径(pcfRadius), z=normalBias, w=strength
    };
    static_assert(sizeof(ShadowConstantBuffer) % 16 == 0, "定数バッファは16バイト区切り");

    int GetCasterCount();   // 登録されている影を落とす物の数(確認用)
    int GetDrawnCount();    // 直前の影のパスで実際に描いた数(確認用。無効の時は0。スポットのパスがあれば、その分も足す)
    bool IsSpotShadowActive();   // 直前のフレームで、スポットライトの影のパスを実行したか(確認用)
}
