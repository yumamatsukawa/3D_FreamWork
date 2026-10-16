#pragma once
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include "Mesh.h"

using namespace DirectX;

// 地形の設定
struct TerrainSettings {
    float width = 10000.f;       // X方向の大きさ
    float depth = 10000.f;       // Z方向の大きさ
    float maxHeight = 1000.f;    // 高さのデータが1(画像なら真っ白)の時の高さ。0(黒)=高さ0
    int   cellsX = 128;          // X方向の分割数(マスの数。頂点は +1 個)。1〜256(範囲外は丸める)
    int   cellsZ = 128;          // Z方向の分割数。1〜256
    float uvRepeat = 20.f;       // 画像を地形全体に何回くり返して貼るか
    std::wstring texture;        // 地形に貼る画像(空なら白)。TerrainRenderer の最初のマテリアルに入る
};

// 地形の高さのデータ。高さの画像(PNG)か表(CSV)から作り、高さ・面の向きを調べたり、描画用のメッシュを作ったりする。
// 描画は TerrainRenderer、物理の当たり判定(6.8)は同じデータ(GetHeights)から作る。
//
// 決まり:
// - 地形の中は「左手前の角が原点(0,0)、X・Zが増える向き」。高さ0が下、Y+が上。ワールドに置く位置は TerrainRenderer のオーナーの位置
// - 高さのデータは (cellsX+1) x (cellsZ+1) 個の頂点。GetHeights()[x * GetVertexCountZ() + z] が頂点 (x, z) の高さ
//   (行=X方向、列=Z方向。PhysXの高さフィールドと同じ並び)。値は maxHeight をかけた後のワールドの高さ
// - 高さの画像の向き: 画像の左が X=0、画像の上が Z=最大(上から見た地図と同じ向き)。画像の大きさと分割数が違う時は、双線形補間で引き伸ばす
// - 1マスは2枚の三角形。対角線は全マスで (x, z) → (x+1, z+1)(PhysXの tessFlag を立てた向き。コライダーと見た目をそろえるため)
// - GetHeightAt / GetNormalAt は、描画のメッシュと同じ三角形の上で計算する(物を地形の上に置いた時に浮かない・埋まらない)
class Terrain {
public:
    // 高さの画像から作る。PNG・BMP・JPEG など。16ビットのグレースケールは65536段階、それ以外は赤(R)の値の256段階を高さにする。
    // 読めなければ nullptr(出力ウィンドウに理由を出す)
    static std::shared_ptr<Terrain> LoadFromImage(const std::wstring& path, const TerrainSettings& settings);

    // 表(CSV)から作る。1行目は列の名前 "x0,x1,x2,...(Csv クラスの決まり)"、2行目からデータ。
    // 1マスに0〜1の数値を1つ(範囲外は丸める)。最初のデータ行が Z=最大(画像の上と同じ向き)、列 x0 が X=0。
    // 読めなければ nullptr
    static std::shared_ptr<Terrain> LoadFromCsv(const std::wstring& path, const TerrainSettings& settings);

    // 0〜1の数値の並びから作る(画像やCSVを使わず、プログラムで地形を作りたい時・確認用)。
    // values は 横 sampleWidth x 縦 sampleHeight 個、画像と同じ並び(最初の行が Z=最大、左から右へ X が増える)。
    // 横・縦は2以上。足りなければ nullptr
    static std::shared_ptr<Terrain> CreateFromSamples(const std::vector<float>& values, int sampleWidth, int sampleHeight,
        const TerrainSettings& settings);

    // 地形の左手前の角を原点(0,0)とした座標で調べる(ワールドへの変換は TerrainRenderer がやる)
    float    GetHeightAt(float localX, float localZ) const;    // その場所の高さ(面の上の高さ)。範囲外は一番近い端の高さ
    XMFLOAT3 GetNormalAt(float localX, float localZ) const;    // その場所の面の向き(長さ1)。描画の法線(頂点ごとになめらか)とは違い、三角形1枚ごとの向き。範囲外は一番近い端の面
    bool     Contains(float localX, float localZ) const;       // 地形の範囲内か

    const TerrainSettings& GetSettings() const { return settings; }
    const std::vector<float>& GetHeights() const { return heights; }   // 頂点の高さ(上の決まり)
    int   GetVertexCountX() const { return settings.cellsX + 1; }
    int   GetVertexCountZ() const { return settings.cellsZ + 1; }
    float GetCellSizeX() const { return settings.width / (float)settings.cellsX; }   // 1マスのX方向の長さ
    float GetCellSizeZ() const { return settings.depth / (float)settings.cellsZ; }
    float GetHeightAtVertex(int x, int z) const;   // 頂点 (x, z) の高さ(範囲外は端)

    // 描画用の頂点とインデックスを作る(TerrainRenderer が使う)。法線は隣の頂点との高さの差(中心差分)から求めてなめらかにする
    void BuildMesh(std::vector<MeshVertex>& vertices, std::vector<UINT>& indices) const;

private:
    Terrain() = default;

    TerrainSettings settings;
    std::vector<float> heights;   // [x * vertexCountZ + z]

    static void SanitizeSettings(TerrainSettings& settings);
    static std::shared_ptr<Terrain> Build(const std::vector<float>& values, int sampleWidth, int sampleHeight, TerrainSettings settings);

    // 座標から、マスの番号(0〜cells-1)とマスの中の位置(0〜1)を求める。範囲外は端のマスに丸める
    struct CellPos { int cx, cz; float fx, fz; };
    CellPos Locate(float localX, float localZ) const;
};
