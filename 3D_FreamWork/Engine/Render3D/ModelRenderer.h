#pragma once
#include "../Core/Component.h"
#include "Model.h"
#include "Material.h"
#include "Shadow.h"
#include "Lod.h"
#include "../Core/Transform.h"
#include <memory>
#include <string>
#include <vector>

// GameObjectに「FBXモデル(複数のパーツ)を描画する見た目」を持たせるComponent。
// モデルのデータ(形・親子関係)は Resources 経由でみんなで共有し、パーツの今の動きはこのComponentが1体ごとに持つ。
// 描画は MeshRenderer と同じ仕組み(Shader3D.hlsl)で、パーツごとに「親→子へ累積したワールド行列」で描く。
// 影: material.castShadow = true にすると、Shadow が有効な時に影を落とす(骨で曲げるパーツも、曲げた後の形で落とす)。
// 影を受けるかは material.receiveShadow(全パーツ共通)。
// 縁の光 / 枠線: SetRimLight / SetOutline(全パーツ共通。どちらも既定はオフ)。枠線はパーツごとに、描いた直後に外側へ描く(骨で曲げるパーツは曲げた後の形)
// 距離で省く(LOD): GetLod().drawDistance / shadowDistance / outlineDistance(MeshRenderer と同じ。既定は0=いつも)。
// 半径は「止まっている姿の一番遠い頂点」×1.5(画面判定と同じ余裕。アニメーションではみ出しても、遠いと誤って省かないように)
class ModelRenderer : public Component, public Shadow::Caster {
private:
    std::shared_ptr<Model> model;              // 読み込んだモデル(共有。読み込めなかった時はnullptr)
    std::vector<Transform> nodeTransforms;     // パーツごとの今の動き(親のパーツからの相対。サイズは作った後に変えない)
    std::vector<unsigned int> nodeTexIDs;      // パーツごとにResourcesから借りている画像(借りていなければUINT_MAX)
    unsigned int overrideTexID = UINT_MAX;     // SetTextureで全パーツに設定した画像(UINT_MAXなら各パーツの画像を使う)
    std::wstring loadedOverrideTexture;        // overrideTexID で借りている画像のパス(material.texture と違っていたら、借り直す)
    Material material;                         // 全パーツ共通の見た目の設定(material.texture が空なら、各パーツの画像を使う)
    std::vector<XMMATRIX> modelMatrices;       // 描画用の作業場所: パーツごとの「モデル空間」の行列(毎フレーム作り直す)
    std::vector<XMMATRIX> boneMatrices;        // 描画用の作業場所: 骨で曲げるパーツのスキン行列
    Material partMaterial;                     // 描画用の作業場所: パーツの色を掛けたmaterial(毎回作り直す)
    bool frustumCulling = false;               // trueなら、画面の外にある時は描かない
    LodSettings lod;                           // 距離で省く設定(既定はすべて使わない)
    bool useModelColors = true;                // trueなら、FBXのマテリアルの色をパーツごとに掛ける

    static constexpr float cullingMargin = 1.5f;   // 画面判定の球に持たせる余裕(骨・アニメーションで、止まっている姿よりはみ出すため)

    // material.texture が変わっていたら、全パーツ共通の画像を借り直す
    void SyncTexture();

    // パーツごとの「モデル空間」の行列を作り直す(modelMatrices。描く時と影を描く時に使う)
    void UpdateModelMatrices();

public:
    // 例: obj->AddComponent<ModelRenderer>(L"Assets/Models/Tank.fbx");
    // 読み込めなかった時は何も描かない(出力ウィンドウに理由が出る)
    explicit ModelRenderer(const std::wstring& path);
    ~ModelRenderer() override;

    // 全パーツに同じ画像を貼る(呼ばなければ、FBXに書かれたパーツごとの画像を Assets/ から探して使う)
    void SetTexture(const std::wstring& path);

    void SetColor(float r, float g, float b, float a) { material.color = { r, g, b, a }; }
    XMFLOAT4 GetColor() const { return material.color; }

    // FBXのマテリアルのdiffuse色を、パーツごとの色として使うか(既定true)。描く色 = パーツの色 × SetColor の色(画像があれば画像 × 色)。
    // マテリアルが無い・色が無い・画像のマテリアルのパーツは白(そのまま)。falseにすると、FBXの色は使わない。枠線の色には影響しない
    void SetUseModelColors(bool enabled) { useModelColors = enabled; }
    bool IsUseModelColors() const { return useModelColors; }

    // マテリアル(色・光沢・自発光・半透明など)を全パーツ共通で設定する。
    // material.texture が空なら各パーツの画像を使い、パスを入れると全パーツにその画像を貼る(SetTextureと同じ)
    void SetMaterial(const Material& value) {
        material = value;
        SyncTexture();
    }
    // 手直し用: modelRenderer->GetMaterial().emissiveStrength = 0.5f;
    Material& GetMaterial() { return material; }
    const Material& GetMaterial() const { return material; }

    // 縁の光(リム発光): 視線に対して斜めの縁を、物の内側で光らせる。strength=0で消す(MeshRenderer と同じ)
    void SetRimLight(XMFLOAT3 color, float strength, float power = 3.f) {
        material.rimColor = color;
        material.rimStrength = strength;
        material.rimPower = power;
    }

    // 枠線(アウトライン): 物の外側に、画面上で widthPixels ピクセルの太さの線を描く(遠くても近くても同じ太さ)。0で消す
    // seeThrough = true にすると、手前に壁などがあって隠れていても枠線が見える(既定false)。その物の SetDrawPriority を大きくしておくこと
    void SetOutline(XMFLOAT4 color, float widthPixels, bool seeThrough = false) {
        material.outlineColor = color;
        material.outlineWidth = widthPixels;
        material.outlineSeeThrough = seeThrough;
    }

    // trueにすると、カメラに映らない(画面の外の)時は描かなくなる(既定はfalse)。物理(当たり判定)は止まらない。
    // 大きさは「止まっている姿の一番遠い頂点」×1.5(余裕)。アニメーションで大きく動く・遠くへはみ出すモデルは、
    // 画面の端で消えて見えることがあるので、その時はfalseに戻す(Visibility::GetCulledCountで確認)
    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }

    // 距離で省く設定(drawDistance / shadowDistance / outlineDistance。0=使わない)。まとめて渡す
    void SetLod(const LodSettings& settings) { lod = settings; }
    // 手直し用: modelRenderer->GetLod().drawDistance = 8000.f;
    LodSettings& GetLod() { return lod; }
    const LodSettings& GetLod() const { return lod; }

    // 名前でパーツを探して、その動き(Transform)を返す。無ければnullptr
    // position / rotate(度数) / scale を書き換えると、そのパーツと子のパーツが動く(砲台だけ回す、など)。
    // 値は「親のパーツからの相対」(一番上のパーツは、このオブジェクトのTransformからの相対)。
    // ※このTransformには親子関係を設定していないので、GetWorldMatrix() は使えない(ローカルの値を読み書きするだけ)
    // アニメーションを再生している間は、ModelAnimationComponent が毎フレームこの値を上書きする(手で動かしても、アニメが動かすパーツは次のフレームにアニメの値に戻る)
    Transform* GetNodeTransform(const std::string& name);

    // 番号でパーツの動きを返す(範囲外はnullptr)。ModelAnimationComponent が使う
    Transform* GetNodeTransform(int index) {
        return (index >= 0 && index < (int)nodeTransforms.size()) ? &nodeTransforms[index] : nullptr;
    }

    // 読み込んだモデル(無ければnullptr)
    const std::shared_ptr<Model>& GetModel() const { return model; }

    void Init() override;
    void Draw() override;
    void Uninit() override;

    // 影のパス: 全パーツの深度だけを描く(Shadow::Render が、アクティブで有効な時だけ呼ぶ)
    void DrawShadow(const Shadow::LightView& light) override;
};
