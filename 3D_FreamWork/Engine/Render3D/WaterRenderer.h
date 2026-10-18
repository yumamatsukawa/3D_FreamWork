#pragma once
#include "../Core/Component.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <cstddef>

using namespace DirectX;

// 水面の設定
struct WaterSettings {
    XMFLOAT2 size{ 10000.f, 10000.f };           // 水面の大きさ(X, Z)
    int      cells = 128;                         // 格子の分割数(1〜512。多いほど波が細かく、頂点が増える)
    XMFLOAT4 color{ 0.1f, 0.4f, 0.6f, 0.6f };    // 水の色(wが透明度。0=透明、1=不透明)
    XMFLOAT4 skyColor{ 0.6f, 0.8f, 1.0f, 1.0f }; // 空の色(浅い角度で映り込む色。反射の簡易表現)
    float    waveHeight = 15.f;                   // 波の高さ(水位から上下に最大この高さまで動く。山の高さの半分)
    float    waveLength = 400.f;                  // 波の長さ(山から山まで。一番大きい波の値)
    float    waveSpeed = 120.f;                   // 波の進む速さ(1秒に進む距離。一番大きい波の値)
    XMFLOAT2 waveDirection{ 1.f, 0.4f };          // 波の進む向き(X, Z。長さは使わない)
    float    specularStrength = 1.0f;             // 太陽(と光)のきらめきの強さ。0=きらめかない
    float    shininess = 128.f;                   // きらめきの鋭さ(大きいほど小さく鋭い)
    float    fresnelPower = 4.f;                  // 浅い角度でどれだけ空が映るか(大きいほど、水面すれすれだけ映る)
    bool     receiveShadow = false;               // true=他の物の影を受ける(影の設定は §32。水面は影を落とさない)
};

constexpr int waterWaveCount = 4;   // 波の本数(Shader3DWater.hlsl の WATER_WAVE_COUNT と同じ)

// シェーダー(Shader3DWater.hlsl の WaterBuffer, b6)に送る形。16バイトずつ区切って並べる
struct WaterConstantBuffer {
    XMFLOAT4 color;                     // rgb=水の色, a=透明度
    XMFLOAT4 sky;                       // rgb=空の色, w=影を受ける(1/0)
    XMFLOAT4 params;                    // x=specularStrength, y=fresnelPower, z=shininess
    XMFLOAT4 size;                      // x=幅(X), y=奥行き(Z)
    XMFLOAT4 waves[waterWaveCount];     // xy=向き(長さ1), z=振幅, w=周波数(2π ÷ 波長)
    XMFLOAT4 phases;                    // 波ごとの位相(0〜2π)
};
static_assert(sizeof(WaterConstantBuffer) == 16 * 9, "Shader3DWater.hlsl の WaterBuffer(float4 が9本)と同じ大きさ");
static_assert(sizeof(WaterConstantBuffer) % 16 == 0, "定数バッファは16バイト区切り");
static_assert(offsetof(WaterConstantBuffer, size) == 16 * 3, "WaterBuffer の waterSize の位置");
static_assert(offsetof(WaterConstantBuffer, waves) == 16 * 4, "WaterBuffer の waves の位置");
static_assert(offsetof(WaterConstantBuffer, phases) == 16 * 8, "WaterBuffer の wavePhases の位置");
static_assert(waterWaveCount == 4, "phases は float4 1本(波4本ぶん)");

// 水面(波打つ半透明の面)を描く Component。オーナーの Transform の位置が水面の中心で、Y が水位(波の高さ0の所)。
// 地形(TerrainRenderer)のくぼみに水を張る、池・湖・海のように使う。
//
// 決まり:
// - オーナーの Transform は位置(と親)だけを使う。拡大縮小・回転は使えない(GetHeightAt と見た目がずれる)ので {1,1,1}・回転なしのままにする
// - 波は、頂点シェーダーが時間から計算する(CPUで頂点を書き換えない)。波は4本(向き・長さ・高さ・速さが少しずつ違う)の合成。
//   時間は Time::GetTime()(倍率の影響を受ける。ポーズ中は波も止まる)。波の式はワールドのXZ座標で決まるので、水面を動かしても波は続く
// - 半透明: 下の物が透けて見える。深度は書かない(水の中の物が消えない)。両面を描く。
//   不透明な物(地形・Player)より後に描くため、オーナーの drawPriority を大きくする(例 GameObject::SetDrawPriority(100))
//   Scene::Draw はカメラからの距離順ではないので、水面と、水面の中の半透明の物(敵の画像など)の前後は正しくならないことがある
// - 光: 平行光源(Image::GetLight())・環境光・点光源・スポットライトを受け、光沢でキラッと光る。縁(浅い角度)は空の色が映る(フレネル)。
//   影は落とさない。receiveShadow = true の時だけ、他の物の影を受ける
// - 水面と地形が交わる所は、くっきり切れて見える(「浅い所は薄い」「岸辺の泡」は未対応)
// - SetFrustumCulling(true) にすると、水面全体(波の高さの余裕込み)を囲む球がカメラに映らない時は描かない(既定はfalse)
// - 当たり判定は付かない。GetHeightAt で波の高さを調べて、物を浮かべることはできる
class WaterRenderer : public Component {
private:
    WaterSettings settings;
    bool ready = false;              // 描ける状態か(失敗したら描かない)
    bool countedInShared = false;    // 共有物(シェーダー・ステート)の数に数えているか
    bool frustumCulling = false;     // trueなら、画面の外にある時は描かない
    ID3D11Buffer* vertexBuffer = nullptr;   // 格子の頂点(-0.5〜0.5のxz)。作る時に1回だけ送る(変更不可)
    ID3D11Buffer* indexBuffer = nullptr;    // 格子の三角形(32ビット)
    UINT indexCount = 0;
    int builtCells = 0;              // いま作ってある格子の分割数(settings.cells が変わったら作り直す)

    bool BuildGrid(int cells);
    void Release();                  // 自分の分を手放す(最後の1体なら共有物も解放する。何回呼んでもよい)

public:
    // 例: water->AddComponent<WaterRenderer>(settings);
    explicit WaterRenderer(const WaterSettings& settings = WaterSettings());
    ~WaterRenderer() override;

    // 実行中に手直しできる(色・波・大きさなど。次のフレームから反映)。cells は変えると格子を作り直す
    WaterSettings& GetSettings() { return settings; }
    const WaterSettings& GetSettings() const { return settings; }

    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }

    // その場所の水面のワールドのY(オーナーのYに波の高さを足した値。シェーダーと同じ式)。水面の範囲の外でも、波の式のまま返す
    float GetHeightAt(float worldX, float worldZ) const;
    // 時間(秒。Time::GetTime() と同じ意味)を指定する版。確認用や、少し先の波を知りたい時に使う
    float GetHeightAt(float worldX, float worldZ, float time) const;
    // 水面の範囲内か(XZだけで調べる。波の高さは見ない)
    bool Contains(float worldX, float worldZ) const;

    void Draw() override;
    void Uninit() override;
};
