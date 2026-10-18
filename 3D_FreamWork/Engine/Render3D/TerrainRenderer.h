#pragma once
#include "../Core/Component.h"
#include "Terrain.h"
#include "Mesh.h"
#include "Material.h"
#include "Shadow.h"
#include "Lod.h"
#include <climits>
#include <memory>
#include <string>

// 地形(Terrain)を描く Component。オーナーの Transform の位置が「地形の左手前の角(X・Zが最小の角)」で、Y がそのまま高さ0。
// 見た目は MeshRenderer と同じ Material で決める(最初は TerrainSettings::texture の画像が入る)。ライト・影・縁の光も MeshRenderer と同じ描画経路で効く。
//
// 決まり:
// - オーナーの Transform は位置(と親)だけを使う。拡大縮小・回転は使えない(GetHeightAt と見た目がずれる)ので {1,1,1}・回転なしのままにする
// - 影: material.receiveShadow = true で影を受ける。material.castShadow = true で影も落とす(地形全体を影のパスで描くので、重くなる。山が大きな影を落とす時だけ)
// - 枠線(SetOutline)は付けない(地形全体の枠線に意味が無いため)。縁の光(rim)はマテリアルの設定で効く
// - SetFrustumCulling(true) にすると、地形全体を囲む球がカメラに映らない時は描かない(既定はfalse。1枚の大きな地形は、たいていどこかが映っている。チャンク分けはしていない)
// - 距離で省く(LOD): GetLod().drawDistance / shadowDistance(既定は0=いつも)。距離は「カメラから地形全体を囲む球の表面まで」なので、地形の中や近くにカメラがいる間は省かれない。
//   枠線は付けないので outlineDistance は使わない。省くのは見た目だけ(当たり判定は変わらない)
// - 1つの地形は1回の描画(頂点とインデックスは作る時に1回だけ送る)。最大 256 x 256 マス(頂点66049個)でも軽い
// - 物理の当たり判定は付かない(6.8 の地形コライダーを使う)。GetHeightAt でその場所の高さを調べて、物を置くことはできる
class TerrainRenderer : public Component, public Shadow::Caster {
private:
    std::shared_ptr<Terrain> terrain;
    std::shared_ptr<Mesh> mesh;
    bool meshReady = false;          // mesh->Init に成功したか(失敗したら描かない・Uninit もしない)
    unsigned int texID = UINT_MAX;   // Resourcesから借りている画像
    std::wstring loadedTexture;      // 借りている画像のパス(material.texture と違っていたら、借り直す)
    Material material;               // 見た目の設定
    bool frustumCulling = false;     // trueなら、画面の外にある時は描かない
    LodSettings lod;                 // 距離で省く設定(既定はすべて使わない)

    void SyncTexture();
    XMFLOAT3 GetOrigin() const;      // 地形の左手前の角のワールド座標
    void GetBoundingSphere(XMFLOAT3& center, float& radius) const;   // 地形の箱をすっぽり囲む球(画面判定と距離の判定に使う)

public:
    // 例: ground->AddComponent<TerrainRenderer>(terrain);
    explicit TerrainRenderer(const std::shared_ptr<Terrain>& terrain);
    ~TerrainRenderer() override;

    // マテリアルをまとめて設定する(画像は material.texture のパスから Resources で読む。空なら画像なし=白)
    void SetMaterial(const Material& value);
    // 手直し用: renderer->GetMaterial().receiveShadow = true;
    Material& GetMaterial() { return material; }
    const Material& GetMaterial() const { return material; }

    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }

    // 距離で省く設定(drawDistance / shadowDistance。0=使わない)。まとめて渡す
    void SetLod(const LodSettings& settings) { lod = settings; }
    // 手直し用: renderer->GetLod().drawDistance = 9000.f;
    LodSettings& GetLod() { return lod; }
    const LodSettings& GetLod() const { return lod; }

    // ワールド座標で調べる(オーナーの位置を足し引きして、Terrain の関数を呼ぶ)
    float    GetHeightAt(float worldX, float worldZ) const;    // その場所の地面のワールドのY(範囲外は一番近い端の高さ)
    XMFLOAT3 GetNormalAt(float worldX, float worldZ) const;    // その場所の面の向き(長さ1)
    bool     Contains(float worldX, float worldZ) const;       // 地形の範囲内か
    const std::shared_ptr<Terrain>& GetTerrain() const { return terrain; }

    void Init() override;
    void DrawShadow(const Shadow::LightView& light) override;
    void Draw() override;
    void Uninit() override;
};
