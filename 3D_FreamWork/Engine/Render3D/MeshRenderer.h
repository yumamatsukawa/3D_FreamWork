#pragma once
#include "../Core/Component.h"
#include "Mesh.h"
#include "Material.h"
#include "../Render/Image.h"
#include "../Core/Resources.h"
#include "../Render/Visibility.h"
#include "Shadow.h"
#include "Lod.h"
#include "RenderStats.h"
#include "../Core/GameObject.h"
#include <cmath>
#include <memory>

// GameObjectに「3Dメッシュを描画する見た目」を持たせるComponent。
// SpriteRendererの3D版。Cameraを使ってWorld/View/Projectionで描画する。
// 見た目(色・画像・光沢・自発光・半透明など)は Material にまとめて持つ。
// SetColor / SetTexture / SetUnlit / SetDepthWrite は、Material の項目を書き換える短い書き方。
// 影: material.castShadow = true にすると、Shadow が有効な時に影を落とす(Shadow に自分を登録しておき、影のパスで深度だけ描く)。
// 影を受けるかは material.receiveShadow。
// 縁の光 / 枠線: SetRimLight / SetOutline(material の rim* / outline* を書き換える短い書き方)。どちらも既定はオフ。
// 枠線は、本物を描いた直後に Mesh::DrawOutline で描く(material.depthWrite = false の物には描かない)。影は落とさず・受けない
// 距離で省く(LOD): GetLod().drawDistance / shadowDistance / outlineDistance を決めると、カメラから遠い物を描かない/影を落とさない/枠線を描かない(既定は0=いつも)。
// 距離は「カメラから物の球の表面まで」(Lod.h)。数は RenderStats / Visibility::GetCulledCount で確認する
class MeshRenderer : public Component, public Shadow::Caster {
private:
    std::shared_ptr<Mesh> mesh;
    unsigned int texID = UINT_MAX;   // Resourcesから借りている画像(同じ画像の人みんなで共有)
    std::wstring loadedTexture;      // 借りている画像のパス(material.texture と違っていたら、借り直す)
    Material material;               // 見た目の設定
    bool frustumCulling = false;     // trueなら、画面の外にある時は描かない
    float boundingRadius = 0.f;      // 画面判定用: 原点から一番遠い頂点までの距離(拡大する前)
    LodSettings lod;                 // 距離で省く設定(既定はすべて使わない)

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

    ~MeshRenderer() override {
        Shadow::Unregister(this);   // Uninit が呼ばれずに壊されても、古いポインタが残らないように
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

    // 縁の光(リム発光): 視線に対して斜めの縁を、物の内側で光らせる。strength=0で消す。power が大きいほど縁の細い線だけが光る
    // 例: meshRenderer->SetRimLight({ 0.3f, 0.7f, 1.f }, 1.5f);
    void SetRimLight(XMFLOAT3 color, float strength, float power = 3.f) {
        material.rimColor = color;
        material.rimStrength = strength;
        material.rimPower = power;
    }

    // 枠線(アウトライン): 物の外側に、画面上で widthPixels ピクセルの太さの線を描く(遠くても近くても同じ太さ)。0で消す
    // 例: meshRenderer->SetOutline({ 1.f, 0.9f, 0.2f, 1.f }, 3.f);
    // seeThrough = true にすると、手前に壁などがあって隠れていても枠線が見える(既定false)。その物の SetDrawPriority を大きくしておくこと
    void SetOutline(XMFLOAT4 color, float widthPixels, bool seeThrough = false) {
        material.outlineColor = color;
        material.outlineWidth = widthPixels;
        material.outlineSeeThrough = seeThrough;
    }

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

    // 距離で省く設定(drawDistance / shadowDistance / outlineDistance。0=使わない)。まとめて渡す
    // 例: meshRenderer->SetLod({ 6000.f, 2500.f, 1500.f });
    void SetLod(const LodSettings& settings) { lod = settings; }
    // 手直し用: meshRenderer->GetLod().drawDistance = 6000.f;
    LodSettings& GetLod() { return lod; }
    const LodSettings& GetLod() const { return lod; }

    // 影を落とす物として登録する(影を落とすかどうかは material.castShadow で、影のパスの度に見る)
    void Init() override {
        Shadow::Register(this, this);
    }

    // 影のパス: 深度だけを描く(Shadow::Render が、アクティブで有効な時だけ呼ぶ)
    void DrawShadow(const Shadow::LightView& light) override {
        if (!material.castShadow) return;
        // 遠い物は影を落とさない
        if (lod.shadowDistance > 0.f && Lod::IsBeyond(GetOwner()->transform.GetWorldMatrix(), boundingRadius, lod.shadowDistance)) {
            RenderStats::AddLodCulled();
            return;
        }
        mesh->DrawDepth(GetOwner()->transform.GetWorldMatrix(), light.view, light.proj);
    }

    void Draw() override {
        // 遠い物は描かない(枠線も描かない)
        if (lod.drawDistance > 0.f && Lod::IsBeyond(GetOwner()->transform.GetWorldMatrix(), boundingRadius, lod.drawDistance)) {
            RenderStats::AddLodCulled();
            return;
        }

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

        // 枠線: 本物を描いた直後に描く(本物を深度に書かない物=スカイボックス・半透明の板には付けない)。遠い物には付けない
        if (material.outlineWidth > 0.f && material.depthWrite) {
            if (lod.outlineDistance > 0.f && Lod::IsBeyond(GetOwner()->transform.GetWorldMatrix(), boundingRadius, lod.outlineDistance)) {
                RenderStats::AddLodCulled();
                return;
            }
            mesh->DrawOutline(GetOwner()->transform.GetWorldMatrix(), Image::GetCamera3D(), material.outlineColor, material.outlineWidth, material.outlineSeeThrough);
        }
    }

    void Uninit() override {
        Shadow::Unregister(this);
        mesh->Uninit();
        // 借りた画像を返す(他に使っている人がいるかもしれないので、ここでは解放しない)
        if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
        texID = UINT_MAX;
        loadedTexture.clear();   // material.texture は残す(使い回されて描く時に、また借りる)
        mesh->SetTexture(nullptr);
    }
};
