#include "TerrainRenderer.h"
#include "GameObject.h"
#include "Image.h"
#include "Resources.h"
#include "Visibility.h"
#include <cmath>

TerrainRenderer::TerrainRenderer(const std::shared_ptr<Terrain>& terrainData) : terrain(terrainData) {
    if (!terrain) {
        OutputDebugStringA("★ TerrainRenderer: Terrain が nullptr です(何も描きません)\n");
        return;
    }
    material.texture = terrain->GetSettings().texture;

    // 頂点とインデックスを作り、1回だけ送る(描く度のコピーはしない)
    std::vector<MeshVertex> vertices;
    std::vector<UINT> indices;
    terrain->BuildMesh(vertices, indices);
    mesh = std::make_shared<Mesh>();
    meshReady = mesh->InitIndexed(vertices, indices);
    if (!meshReady) OutputDebugStringA("★ TerrainRenderer: メッシュの作成に失敗\n");
    else SyncTexture();
}

TerrainRenderer::~TerrainRenderer() {
    Shadow::Unregister(this);   // Uninit が呼ばれずに壊されても、古いポインタが残らないように
}

// material.texture が変わっていたら、画像を借り直す(新しい画像を先に借りてから、前の画像を返す。同じ画像は読み直さない)
void TerrainRenderer::SyncTexture() {
    if (!meshReady || material.texture == loadedTexture) return;
    unsigned int newTexID = material.texture.empty() ? UINT_MAX : Resources::LoadTexture(material.texture);
    if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
    texID = newTexID;
    loadedTexture = material.texture;
    mesh->SetTexture(texID != UINT_MAX ? Image::GetTextureView(texID) : nullptr);
}

void TerrainRenderer::SetMaterial(const Material& value) {
    material = value;
    SyncTexture();
}

XMFLOAT3 TerrainRenderer::GetOrigin() const {
    // 親がいる時も正しいよう、ワールド行列の位置を使う
    XMFLOAT3 origin;
    XMStoreFloat3(&origin, GetOwner()->transform.GetWorldMatrix().r[3]);
    return origin;
}

float TerrainRenderer::GetHeightAt(float worldX, float worldZ) const {
    if (!terrain) return 0.f;
    XMFLOAT3 origin = GetOrigin();
    return origin.y + terrain->GetHeightAt(worldX - origin.x, worldZ - origin.z);
}

XMFLOAT3 TerrainRenderer::GetNormalAt(float worldX, float worldZ) const {
    if (!terrain) return { 0.f, 1.f, 0.f };
    XMFLOAT3 origin = GetOrigin();
    return terrain->GetNormalAt(worldX - origin.x, worldZ - origin.z);
}

bool TerrainRenderer::Contains(float worldX, float worldZ) const {
    if (!terrain) return false;
    XMFLOAT3 origin = GetOrigin();
    return terrain->Contains(worldX - origin.x, worldZ - origin.z);
}

// 影を落とす物として登録する(影を落とすかどうかは material.castShadow で、影のパスの度に見る)
void TerrainRenderer::Init() {
    Shadow::Register(this, this);
}

// 影のパス: 深度だけを描く(Shadow::Render が、アクティブで有効な時だけ呼ぶ)
void TerrainRenderer::DrawShadow(const Shadow::LightView& light) {
    if (!meshReady || !material.castShadow) return;
    mesh->DrawDepth(GetOwner()->transform.GetWorldMatrix(), light.view, light.proj);
}

void TerrainRenderer::Draw() {
    if (!meshReady) return;

    // 画面の外なら描かない: 地形の箱(角から幅・高さ・奥行き)をすっぽり囲む球で調べる
    if (frustumCulling && material.depthWrite) {
        const TerrainSettings& s = terrain->GetSettings();
        float halfX = s.width * 0.5f, halfY = fabsf(s.maxHeight) * 0.5f, halfZ = s.depth * 0.5f;
        XMFLOAT3 origin = GetOrigin();
        XMFLOAT3 center = { origin.x + halfX, origin.y + halfY, origin.z + halfZ };
        float radius = sqrtf(halfX * halfX + halfY * halfY + halfZ * halfZ);
        if (!Visibility::IsSphereInView3D(center, radius)) {
            Visibility::CountCulled();
            return;
        }
    }

    // material.texture が手で書き換えられていたら、ここで画像を借り直す
    SyncTexture();

    // 位置(と親)だけを使う。拡大縮小・回転は使えない
    mesh->Draw(GetOwner()->transform, Image::GetCamera3D(), material, Image::GetLight());
}

void TerrainRenderer::Uninit() {
    Shadow::Unregister(this);
    if (!meshReady) return;
    mesh->Uninit();
    meshReady = false;
    // 借りた画像を返す(他に使っている人がいるかもしれないので、ここでは解放しない)
    if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
    texID = UINT_MAX;
    loadedTexture.clear();
    mesh->SetTexture(nullptr);
}
