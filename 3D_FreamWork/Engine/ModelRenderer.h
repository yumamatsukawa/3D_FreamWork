#pragma once
#include "Component.h"
#include "Model.h"
#include "Transform.h"
#include <memory>
#include <string>
#include <vector>

// GameObjectに「FBXモデル(複数のパーツ)を描画する見た目」を持たせるComponent。
// モデルのデータ(形・親子関係)は Resources 経由でみんなで共有し、パーツの今の動きはこのComponentが1体ごとに持つ。
// 描画は MeshRenderer と同じ仕組み(Shader3D.hlsl)で、パーツごとに「親→子へ累積したワールド行列」で描く
class ModelRenderer : public Component {
private:
    std::shared_ptr<Model> model;              // 読み込んだモデル(共有。読み込めなかった時はnullptr)
    std::vector<Transform> nodeTransforms;     // パーツごとの今の動き(親のパーツからの相対。サイズは作った後に変えない)
    std::vector<unsigned int> nodeTexIDs;      // パーツごとにResourcesから借りている画像(借りていなければUINT_MAX)
    unsigned int overrideTexID = UINT_MAX;     // SetTextureで全パーツに設定した画像(UINT_MAXなら各パーツの画像を使う)
    XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };
    std::vector<XMMATRIX> modelMatrices;       // 描画用の作業場所: パーツごとの「モデル空間」の行列(毎フレーム作り直す)
    std::vector<XMMATRIX> boneMatrices;        // 描画用の作業場所: 骨で曲げるパーツのスキン行列
    bool frustumCulling = false;               // trueなら、画面の外にある時は描かない

    static constexpr float cullingMargin = 1.5f;   // 画面判定の球に持たせる余裕(骨・アニメーションで、止まっている姿よりはみ出すため)

public:
    // 例: obj->AddComponent<ModelRenderer>(L"Assets/Models/Tank.fbx");
    // 読み込めなかった時は何も描かない(出力ウィンドウに理由が出る)
    explicit ModelRenderer(const std::wstring& path);

    // 全パーツに同じ画像を貼る(呼ばなければ、FBXに書かれたパーツごとの画像を Assets/ から探して使う)
    void SetTexture(const std::wstring& path);

    void SetColor(float r, float g, float b, float a) { color = { r, g, b, a }; }
    XMFLOAT4 GetColor() const { return color; }

    // trueにすると、カメラに映らない(画面の外の)時は描かなくなる(既定はfalse)。物理(当たり判定)は止まらない。
    // 大きさは「止まっている姿の一番遠い頂点」×1.5(余裕)。アニメーションで大きく動く・遠くへはみ出すモデルは、
    // 画面の端で消えて見えることがあるので、その時はfalseに戻す(Visibility::GetCulledCountで確認)
    void SetFrustumCulling(bool enabled) { frustumCulling = enabled; }
    bool IsFrustumCulling() const { return frustumCulling; }

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
};
