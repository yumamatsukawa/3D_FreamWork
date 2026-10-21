#include "ModelRenderer.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"
#include "../Core/Resources.h"
#include "../Render/Visibility.h"
#include "RenderStats.h"
#include <climits>

ModelRenderer::ModelRenderer(const std::wstring& path) {
    model = Resources::LoadModel(path);
    if (!model) return;

    // パーツごとの動きを、モデルが持っている最初の位置・回転・スケールで初期化する
    int count = model->GetNodeCount();
    nodeTransforms.resize(count);
    nodeTexIDs.assign(count, UINT_MAX);
    for (int i = 0; i < count; i++) {
        const ModelNode& node = model->GetNode(i);
        nodeTransforms[i].position = node.position;
        nodeTransforms[i].rotate = node.rotate;
        nodeTransforms[i].scale = node.scale;
    }
}

ModelRenderer::~ModelRenderer() {
    Shadow::Unregister(this);   // Uninit が呼ばれずに壊されても、古いポインタが残らないように
}

void ModelRenderer::Init() {
    Shadow::Register(this, this);   // 影を落とす物として登録する(落とすかどうかは material.castShadow)
    if (!model) return;

    // FBXに書かれたパーツごとの画像を借りる(同じ画像はResourcesが使い回す)
    for (int i = 0; i < model->GetNodeCount(); i++) {
        const std::wstring& texturePath = model->GetNode(i).texturePath;
        if (!texturePath.empty()) nodeTexIDs[i] = Resources::LoadTexture(texturePath);
    }
}

void ModelRenderer::SetTexture(const std::wstring& path) {
    material.texture = path;
    SyncTexture();
}

void ModelRenderer::SyncTexture() {
    if (material.texture == loadedOverrideTexture) return;

    // 新しい画像を先に借りてから、前の画像を返す(同じ画像を設定し直しても読み直さないように)
    unsigned int newTexID = material.texture.empty() ? UINT_MAX : Resources::LoadTexture(material.texture);
    if (overrideTexID != UINT_MAX) Resources::ReleaseTexture(overrideTexID);
    overrideTexID = newTexID;
    loadedOverrideTexture = material.texture;
}

Transform* ModelRenderer::GetNodeTransform(const std::string& name) {
    if (!model) return nullptr;
    int index = model->FindNode(name);
    if (index < 0) return nullptr;
    return &nodeTransforms[index];
}

void ModelRenderer::UpdateModelMatrices() {
    // パーツごとの「モデル空間」の行列: 自分のローカル行列 × 親の行列(親は必ず先に並んでいる)
    int count = model->GetNodeCount();
    modelMatrices.resize(count);
    for (int i = 0; i < count; i++) {
        int parent = model->GetNode(i).parent;
        modelMatrices[i] = nodeTransforms[i].GetLocalMatrix();
        if (parent >= 0) modelMatrices[i] = modelMatrices[i] * modelMatrices[parent];
    }
}

void ModelRenderer::Draw() {
    if (!model) return;

    // 遠い物は描かない(枠線も描かない)
    float lodRadius = model->GetBoundingRadius() * cullingMargin;
    if (lod.drawDistance > 0.f && Lod::IsBeyond(GetOwner()->transform.GetWorldMatrix(), lodRadius, lod.drawDistance)) {
        RenderStats::AddLodCulled();
        return;
    }

    // material.texture が手で書き換えられていたら、ここで画像を借り直す
    SyncTexture();

    // 画面の外なら描かない
    if (frustumCulling &&
        !Visibility::IsTransformedSphereInView3D(GetOwner()->transform.GetWorldMatrix(), model->GetBoundingRadius() * cullingMargin)) {
        Visibility::CountCulled();
        return;
    }

    // オーナーのワールド行列は、描く時に最後に掛ける(骨で曲げるメッシュは、モデル空間の骨の行列を使うため)
    XMMATRIX ownerWorld = GetOwner()->transform.GetWorldMatrix();
    UpdateModelMatrices();
    int count = model->GetNodeCount();

    // 枠線: パーツを描いた直後に、そのパーツの外側へ描く(本物を深度に書かない物には付けない)
    bool drawOutline = material.outlineWidth > 0.f && material.depthWrite;
    if (drawOutline && lod.outlineDistance > 0.f && Lod::IsBeyond(ownerWorld, lodRadius, lod.outlineDistance)) {
        drawOutline = false;   // 遠い物には枠線を付けない
        RenderStats::AddLodCulled();
    }

    // 形のあるパーツを描く。3D専用カメラ(GetCamera3D)とシーン共通の光源を使うのはMeshRendererと同じ
    for (int i = 0; i < count; i++) {
        const ModelNode& node = model->GetNode(i);
        unsigned int texID = (overrideTexID != UINT_MAX) ? overrideTexID : nodeTexIDs[i];
        ID3D11ShaderResourceView* texture = (texID != UINT_MAX) ? Image::GetTextureView(texID) : nullptr;

        // パーツの色: FBXのマテリアルの色を、SetColor の色に掛ける(白のパーツは、共通のmaterialをそのまま使う)
        const Material* drawMaterial = &material;
        if (useModelColors && (node.color.x != 1.f || node.color.y != 1.f || node.color.z != 1.f || node.color.w != 1.f)) {
            partMaterial = material;
            partMaterial.color = { material.color.x * node.color.x, material.color.y * node.color.y,
                                   material.color.z * node.color.z, material.color.w * node.color.w };
            drawMaterial = &partMaterial;
        }

        // 骨で曲げるパーツ: 骨のパーツの今の行列から、スキン行列(逆バインド行列 × 骨の行列)を作って送る。
        // パーツ自身の位置は使わない(頂点が骨に付いて動くため)。骨のパーツ自体は形が無いので描かれない
        if (node.IsSkinned()) {
            SkinMesh* skinMesh = model->GetSkinMesh(i);
            if (skinMesh) {
                boneMatrices.resize(node.skinBones.size());
                for (size_t k = 0; k < node.skinBones.size(); k++) {
                    boneMatrices[k] = XMLoadFloat4x4(&node.skinInverseBind[k]) * modelMatrices[node.skinBones[k]];
                }
                skinMesh->SetTexture(texture);
                skinMesh->Draw(ownerWorld, boneMatrices.data(), (int)boneMatrices.size(), Image::GetCamera3D(), *drawMaterial, Image::GetLight());
                if (drawOutline) {
                    skinMesh->DrawOutline(ownerWorld, boneMatrices.data(), (int)boneMatrices.size(), Image::GetCamera3D(), material.outlineColor, material.outlineWidth, material.outlineSeeThrough);
                }
                continue;
            }
            // スキンメッシュを作れなかった時は、静止のメッシュとして表示する(下へ)
        }

        Mesh* mesh = model->GetMesh(i);
        if (!mesh) continue;
        mesh->SetTexture(texture);
        mesh->Draw(modelMatrices[i] * ownerWorld, Image::GetCamera3D(), *drawMaterial, Image::GetLight());
        if (drawOutline) mesh->DrawOutline(modelMatrices[i] * ownerWorld, Image::GetCamera3D(), material.outlineColor, material.outlineWidth, material.outlineSeeThrough);
    }
}

void ModelRenderer::DrawShadow(const Shadow::LightView& light) {
    if (!model || !material.castShadow) return;

    // 遠い物は影を落とさない(骨の行列の計算も省ける)
    XMMATRIX ownerWorld = GetOwner()->transform.GetWorldMatrix();
    if (lod.shadowDistance > 0.f && Lod::IsBeyond(ownerWorld, model->GetBoundingRadius() * cullingMargin, lod.shadowDistance)) {
        RenderStats::AddLodCulled();
        return;
    }

    // 描く時と同じ行列で、深度だけを描く(骨で曲げるパーツは、曲げた後の形で影になる)
    UpdateModelMatrices();
    int count = model->GetNodeCount();
    for (int i = 0; i < count; i++) {
        const ModelNode& node = model->GetNode(i);
        if (node.IsSkinned()) {
            SkinMesh* skinMesh = model->GetSkinMesh(i);
            if (skinMesh) {
                boneMatrices.resize(node.skinBones.size());
                for (size_t k = 0; k < node.skinBones.size(); k++) {
                    boneMatrices[k] = XMLoadFloat4x4(&node.skinInverseBind[k]) * modelMatrices[node.skinBones[k]];
                }
                skinMesh->DrawDepth(ownerWorld, boneMatrices.data(), (int)boneMatrices.size(), light.view, light.proj);
                continue;
            }
            // スキンメッシュを作れなかった時は、静止のメッシュとして落とす(下へ)
        }

        Mesh* mesh = model->GetMesh(i);
        if (!mesh) continue;
        mesh->DrawDepth(modelMatrices[i] * ownerWorld, light.view, light.proj);
    }
}

void ModelRenderer::Uninit() {
    Shadow::Unregister(this);
    // 借りた画像を返す(他に使っている人がいるかもしれないので、ここでは解放しない)
    for (unsigned int& texID : nodeTexIDs) {
        if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
        texID = UINT_MAX;
    }
    if (overrideTexID != UINT_MAX) Resources::ReleaseTexture(overrideTexID);
    overrideTexID = UINT_MAX;
    loadedOverrideTexture.clear();   // material.texture は残す(使い回されて描く時に、また借りる)
}
