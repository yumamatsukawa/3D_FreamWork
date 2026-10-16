#pragma once
#include "Component.h"
#include "Mesh.h"
#include "Material.h"
#include "Image.h"
#include "Resources.h"
#include "Visibility.h"
#include "GameObject.h"
#include <cmath>
#include <memory>

// GameObjectに「3Dメッシュを描画する見た目」を持たせるComponent。
// SpriteRendererの3D版。Cameraを使ってWorld/View/Projectionで描画する。
// 見た目(色・画像・光沢・自発光・半透明など)は Material にまとめて持つ。
// SetColor / SetTexture / SetUnlit / SetDepthWrite は、Material の項目を書き換える短い書き方
class MeshRenderer : public Component {
private:
    std::shared_ptr<Mesh> mesh;
    unsigned int texID = UINT_MAX;   // Resourcesから借りている画像(同じ画像の人みんなで共有)
    std::wstring loadedTexture;      // 借りている画像のパス(material.texture と違っていたら、借り直す)
    Material material;               // 見た目の設定
    bool frustumCulling = false;     // trueなら、画面の外にある時は描かない
    float boundingRadius = 0.f;      // 画面判定用: 原点から一番遠い頂点までの距離(拡大する前)

    // material.texture が変わっていたら、画像を借り直す(新しい画像を先に借りてから、前の画像を返す。同じ画像は読み直さない)
    void SyncTexture() {
        if (material.texture == loadedTexture) return;
        unsigned int newTexID = material.texture.empty() ? UINT_MAX : Resources::LoadTexture(material.texture);
        if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
        texID = newTexID;
        loadedTexture = material.texture;
        mesh->SetTexture(texID != UINT_MAX ? Image::GetTextureView(texID) : nullptr);
    }

public:
    // 例: obj->AddComponent<MeshRenderer>(Mesh::CreateCube());
    explicit MeshRenderer(const std::vector<MeshVertex>& vertices) {
        mesh = std::make_shared<Mesh>();
        mesh->Init(vertices);

        // 画面判定の球の半径(立方体なら約0.87、球なら0.5)
        float maxLengthSq = 0.f;
        for (const MeshVertex& v : vertices) {
            float lengthSq = v.Position.x * v.Position.x + v.Position.y * v.Position.y + v.Position.z * v.Position.z;
            if (lengthSq > maxLengthSq) maxLengthSq = lengthSq;
        }
        boundingRadius = sqrtf(maxLengthSq);
    }

    // マテリアルをまとめて設定する(画像は material.texture のパスから Resources で読む。空なら画像なし=白)
    void SetMaterial(const Material& value) {
        material = value;
        SyncTexture();
    }
    // 手直し用: meshRenderer->GetMaterial().specularStrength = 0.8f;
    // (material.texture を書き換えた時は、次に描く時に画像が切り替わる)
    Material& GetMaterial() { return material; }
    const Material& GetMaterial() const { return material; }

    void SetTexture(const std::wstring& filepath) {
        material.texture = filepath;
        SyncTexture();
    }

    void SetColor(float r, float g, float b, float a) { material.color = { r, g, b, a }; }
    DirectX::XMFLOAT4 GetColor() const { return material.color; }

    // スカイボックスなど、光源の影響を受けたくない(常に一定の明るさで表示したい)場合に使う(material.lit = !value)
    void SetUnlit(bool value) { material.lit = !value; }

    // falseにすると、深度バッファに書き込まない(自分より後ろのものを隠さなくなる)。
    // スカイボックス用: GetOwner()->SetDrawPriority()で必ず最初に描画させた上でこれをfalseにする
    void SetDepthWrite(bool value) { material.depthWrite = value; }

    // trueにすると、カメラに映らない(画面の外の)時は描かなくなる(既定はfalse)。
    // 物理(当たり判定)は止まらない。SetDepthWrite(false)(スカイボックスなど画面全体を覆う物)の時は、trueでも判定しない。
    // 大きさは「頂点の一番遠い点」から求める。物が画面の端で消えて見えたら、falseに戻す(Visibility::GetCulledCountで確認)
    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }

    void Draw() override {
        // 画面の外なら描かない(スカイボックスのように深度を書かない物は、画面全体を覆うので判定しない)
        if (frustumCulling && material.depthWrite &&
            !Visibility::IsTransformedSphereInView3D(GetOwner()->transform.GetWorldMatrix(), boundingRadius)) {
            Visibility::CountCulled();
            return;
        }

        // material.texture が手で書き換えられていたら、ここで画像を借り直す
        SyncTexture();

        // Transform::GetWorldMatrix()が親子階層を内部で処理してくれるので、
        // ここではローカルのtransformをそのまま渡せばよい。
        // カメラは2D(GetCamera)とは別の3D専用カメラ(GetCamera3D)を使う。
        // 光を使うかどうか(unlit)は、material.lit がシェーダーに伝える
        mesh->Draw(GetOwner()->transform, Image::GetCamera3D(), material, Image::GetLight());
    }

    void Uninit() override {
        mesh->Uninit();
        // 借りた画像を返す(他に使っている人がいるかもしれないので、ここでは解放しない)
        if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
        texID = UINT_MAX;
        loadedTexture.clear();   // material.texture は残す(使い回されて描く時に、また借りる)
        mesh->SetTexture(nullptr);
    }
};
