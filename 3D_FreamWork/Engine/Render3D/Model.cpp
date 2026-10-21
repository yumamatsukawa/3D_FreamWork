#include "Model.h"
#include "ufbx.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <fstream>
#include <unordered_map>

namespace {
    // UTF-8 → ワイド文字(FBXの中の名前・パスは UTF-8)
    std::wstring ToWide(const char* text, size_t length) {
        if (length == 0) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, nullptr, 0);
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text, (int)length, &result[0], size);
        return result;
    }

    // 拡張子を小文字で返す(".fbx" のように点付き。無ければ空)
    std::wstring GetExtension(const std::wstring& path) {
        size_t dot = path.find_last_of(L'.');
        size_t slash = path.find_last_of(L"/\\");
        if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash)) return L"";
        std::wstring ext = path.substr(dot);
        for (wchar_t& c : ext) c = (wchar_t)std::towlower(c);
        return ext;
    }

    // ファイルを丸ごと読む(日本語を含むパスでも開けるように、ufbxのファイル読みは使わない)
    bool ReadAllBytes(const std::wstring& path, std::vector<char>& data) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        std::streamsize size = file.tellg();
        const std::streamsize maxSize = 512LL * 1024 * 1024;   // 512MBを超えるFBXは読まない(壊れたファイル対策)
        if (size < 0 || size > maxSize) return false;
        file.seekg(0, std::ios::beg);
        data.resize((size_t)size);
        return size == 0 || (bool)file.read(data.data(), size);
    }

    // 回転(クォータニオン)を、Transform::rotate と同じ向きのオイラー角(度)にする。
    // Transform::GetLocalMatrix は XMMatrixRotationRollPitchYaw(x=Pitch, y=Yaw, z=Roll) なので、その逆を求める
    XMFLOAT3 QuaternionToEuler(ufbx_quat q) {
        return Model::QuaternionToEuler({ (float)q.x, (float)q.y, (float)q.z, (float)q.w });
    }
}

XMFLOAT3 Model::QuaternionToEuler(const XMFLOAT4& q) {
    {
        XMVECTOR quat = XMLoadFloat4(&q);
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, XMMatrixRotationQuaternion(quat));

        float pitch, yaw, roll;
        float sinPitch = (std::max)(-1.f, (std::min)(1.f, -m._32));
        pitch = asinf(sinPitch);
        if (fabsf(sinPitch) > 0.99999f) {
            // ちょうど真上・真下を向く時は、ロールが決まらないので0にする
            roll = 0.f;
            yaw = atan2f(-m._13, m._11);
        }
        else {
            yaw = atan2f(m._31, m._33);
            roll = atan2f(m._12, m._22);
        }
        return { XMConvertToDegrees(pitch), XMConvertToDegrees(yaw), XMConvertToDegrees(roll) };
    }
}

namespace {
    // パーツの画像のパスを求める(FBXに書かれたファイル名だけを使い、Assets/ の中から探す)
    std::wstring FindTexturePath(const ufbx_node* node) {
        auto pickFrom = [](const ufbx_material_list& materials) -> std::wstring {
            for (size_t i = 0; i < materials.count; i++) {
                const ufbx_material* material = materials.data[i];
                if (!material) continue;
                const ufbx_texture* texture = material->fbx.diffuse_color.texture;
                if (!texture) texture = material->pbr.base_color.texture;
                if (!texture) continue;

                ufbx_string name = texture->filename.length > 0 ? texture->filename : texture->relative_filename;
                std::wstring file = ToWide(name.data, name.length);
                size_t slash = file.find_last_of(L"/\\");
                if (slash != std::wstring::npos) file = file.substr(slash + 1);
                if (!file.empty()) return L"Assets/" + file;
            }
            return L"";
        };

        std::wstring result = pickFrom(node->materials);
        if (result.empty() && node->mesh) result = pickFrom(node->mesh->materials);
        return result;
    }

    // パーツの色を求める(マテリアルのdiffuse色 × diffuseの係数。画像があるマテリアル・色が無いマテリアルは白)
    XMFLOAT4 FindMaterialColor(const ufbx_node* node) {
        auto pickFrom = [](const ufbx_material_list& materials, XMFLOAT4& out) -> bool {
            for (size_t i = 0; i < materials.count; i++) {
                const ufbx_material* material = materials.data[i];
                if (!material) continue;
                // 画像が貼ってあるなら、画像の色をそのまま見せたいので白のまま
                if (material->fbx.diffuse_color.texture || material->pbr.base_color.texture) return false;
                if (material->fbx.diffuse_color.has_value) {
                    float factor = material->fbx.diffuse_factor.has_value ? (float)material->fbx.diffuse_factor.value_real : 1.f;
                    ufbx_vec3 c = material->fbx.diffuse_color.value_vec3;
                    out = { (float)c.x * factor, (float)c.y * factor, (float)c.z * factor, 1.f };
                    return true;
                }
                if (material->pbr.base_color.has_value) {
                    float factor = material->pbr.base_factor.has_value ? (float)material->pbr.base_factor.value_real : 1.f;
                    ufbx_vec3 c = material->pbr.base_color.value_vec3;
                    out = { (float)c.x * factor, (float)c.y * factor, (float)c.z * factor, 1.f };
                    return true;
                }
                return false;   // 色が書かれていないマテリアル
            }
            return false;
        };

        XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };
        if (!pickFrom(node->materials, color) && node->mesh) pickFrom(node->mesh->materials, color);
        return color;
    }

    // スキンの骨の番号と影響度を頂点に入れる(影響の大きい上位4本だけ使い、合計が1になるように直す)
    void FillSkin(const ufbx_skin_deformer* skin, uint32_t vertexId, SkinVertex& v) {
        for (int k = 0; k < 4; k++) v.BoneIndices[k] = 0;
        v.BoneWeights = { 0.f, 0.f, 0.f, 0.f };
        if (vertexId >= skin->vertices.count) { v.BoneWeights.x = 1.f; return; }

        // この頂点に影響する骨のうち、影響度の大きい上位4つだけを使う
        ufbx_skin_vertex info = skin->vertices.data[vertexId];
        uint32_t cluster[4] = {};
        float weight[4] = {};
        int count = 0;
        for (uint32_t w = 0; w < info.num_weights; w++) {
            const ufbx_skin_weight& sw = skin->weights.data[info.weight_begin + w];
            float value = (float)sw.weight;
            if (value <= 0.f) continue;
            // 4つ未満なら末尾に足し、4つ埋まっていれば一番小さい物より大きい時だけ入れ替える
            int slot = count;
            if (count == 4) {
                slot = 0;
                for (int k = 1; k < 4; k++) if (weight[k] < weight[slot]) slot = k;
                if (weight[slot] >= value) continue;
            }
            else {
                count++;
            }
            cluster[slot] = sw.cluster_index;
            weight[slot] = value;
        }

        // 影響度の合計が1になるように直す(骨に影響されない頂点は、最初の骨に100%)
        float total = 0.f;
        for (int k = 0; k < count; k++) total += weight[k];
        if (count == 0 || total <= 0.f) { v.BoneWeights.x = 1.f; return; }
        float inv = 1.f / total;
        float* out[4] = { &v.BoneWeights.x, &v.BoneWeights.y, &v.BoneWeights.z, &v.BoneWeights.w };
        for (int k = 0; k < count; k++) {
            v.BoneIndices[k] = cluster[k];
            *out[k] = weight[k] * inv;
        }
    }

    // メッシュの面を三角形にして、MeshVertex形式の頂点にする(skinがあれば、骨の影響つきの頂点も同じ並びで作る)
    void BuildVertices(const ufbx_mesh* mesh, std::vector<MeshVertex>& out, const ufbx_skin_deformer* skin, std::vector<SkinVertex>& skinOut) {
        std::vector<uint32_t> triangleIndices(mesh->max_face_triangles * 3);

        for (size_t faceIndex = 0; faceIndex < mesh->num_faces; faceIndex++) {
            if (faceIndex < mesh->face_hole.count && mesh->face_hole.data[faceIndex]) continue;
            ufbx_face face = mesh->faces.data[faceIndex];
            if (face.num_indices < 3) continue;

            // 四角形以上の面は、ufbxが三角形に分けてくれる
            uint32_t triangleCount = ufbx_triangulate_face(triangleIndices.data(), triangleIndices.size(), mesh, face);
            for (uint32_t k = 0; k < triangleCount * 3; k++) {
                size_t index = triangleIndices[k];

                MeshVertex v{};
                ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
                v.Position = { (float)p.x, (float)p.y, (float)p.z };
                if (mesh->vertex_normal.exists) {
                    ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
                    v.Normal = { (float)n.x, (float)n.y, (float)n.z };
                }
                else {
                    v.Normal = { 0.f, 1.f, 0.f };
                }
                v.Color = { 1.f, 1.f, 1.f, 1.f };
                if (mesh->vertex_uv.exists) {
                    ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);
                    v.TexCoord = { (float)uv.x, 1.f - (float)uv.y };  // FBXのVは下から上、このエンジンは上から下が基準なので反転する(OBJと同じ)
                }
                out.push_back(v);

                if (skin) {
                    SkinVertex sv{};
                    sv.Position = v.Position; sv.Normal = v.Normal; sv.Color = v.Color; sv.TexCoord = v.TexCoord;
                    FillSkin(skin, mesh->vertex_indices.data[index], sv);
                    skinOut.push_back(sv);
                }
            }
        }
    }

    // パーツを1つ足して、子のパーツも続けて足していく(親が必ず先に並ぶ)
    void AddNode(const ufbx_node* source, int parent, std::vector<ModelNode>& nodes, std::vector<const ufbx_node*>& sources) {
        ModelNode node;
        node.name = std::string(source->name.data, source->name.length);
        node.parent = parent;

        // 親からの相対の位置・回転・スケール
        const ufbx_transform& t = source->local_transform;
        node.position = { (float)t.translation.x, (float)t.translation.y, (float)t.translation.z };
        node.rotate = QuaternionToEuler(t.rotation);
        node.scale = { (float)t.scale.x, (float)t.scale.y, (float)t.scale.z };

        if (source->mesh) {
            // 骨で曲げるメッシュ(スキン)なら、骨の影響も一緒に読む。骨が多すぎるメッシュは、静止のメッシュとして扱う
            const ufbx_skin_deformer* skin = nullptr;
            if (source->mesh->skin_deformers.count > 0) {
                const ufbx_skin_deformer* candidate = source->mesh->skin_deformers.data[0];
                // 骨が0本のスキンは、静止のメッシュとして扱う(skinはnullptrのまま)
                if (candidate->clusters.count == 0) {
                    // 何もしない
                }
                else if (candidate->clusters.count > (size_t)SkinMesh::maxBones) {
                    OutputDebugStringA(("★ Model::Load: 骨が多すぎるので(最大" + std::to_string(SkinMesh::maxBones) + "本)、曲げずに静止のまま表示します: " + node.name + "\n").c_str());
                }
                else {
                    skin = candidate;
                }
            }
            BuildVertices(source->mesh, node.vertices, skin, node.skinVertices);
            node.texturePath = FindTexturePath(source);
            node.color = FindMaterialColor(source);
        }

        int index = (int)nodes.size();
        nodes.push_back(std::move(node));
        sources.push_back(source);
        for (size_t i = 0; i < source->children.count; i++) {
            AddNode(source->children.data[i], index, nodes, sources);
        }
    }

    // アニメーションのスタック(Mayaで付けたクリップ)ごとに、全パーツの動きを一定の細かさで読み取る。
    // ufbx_evaluate_transform は、読み込み時の単位・座標系の変換を済ませた値を返す(静止モデルのlocal_transformと同じ空間)
    void BuildClips(const ufbx_scene* scene, const std::vector<const ufbx_node*>& sources, std::vector<ModelClip>& clips) {
        const int maxFrames = 20000;   // 長すぎるクリップでメモリを使い切らないための上限
        for (size_t s = 0; s < scene->anim_stacks.count; s++) {
            const ufbx_anim_stack* stack = scene->anim_stacks.data[s];
            ModelClip clip;
            clip.name = std::string(stack->name.data, stack->name.length);
            double length = stack->time_end - stack->time_begin;
            if (length < 0.0) length = 0.0;
            clip.duration = (float)length;
            clip.frameCount = (int)ceil(length * clip.fps - 1e-4) + 1;
            if (clip.frameCount > maxFrames) {
                OutputDebugStringA(("★ Model::Load: クリップが長すぎるので途中までにします: " + clip.name + "\n").c_str());
                clip.frameCount = maxFrames;
                clip.duration = (float)(maxFrames - 1) / clip.fps;
            }

            clip.nodePoses.resize(sources.size());
            for (size_t n = 0; n < sources.size(); n++) {
                clip.nodePoses[n].resize(clip.frameCount);
                for (int f = 0; f < clip.frameCount; f++) {
                    double time = stack->time_begin + (double)f / clip.fps;
                    if (time > stack->time_end) time = stack->time_end;
                    ufbx_transform t = ufbx_evaluate_transform(stack->anim, sources[n], time);
                    ModelPose& pose = clip.nodePoses[n][f];
                    pose.position = { (float)t.translation.x, (float)t.translation.y, (float)t.translation.z };
                    pose.rotation = { (float)t.rotation.x, (float)t.rotation.y, (float)t.rotation.z, (float)t.rotation.w };
                    pose.scale = { (float)t.scale.x, (float)t.scale.y, (float)t.scale.z };
                }
            }
            clips.push_back(std::move(clip));
        }
    }

    // スキンのメッシュに、骨のパーツの番号と逆バインド行列を入れる(全パーツが並んだ後でないと、骨の番号が分からない)
    void FinishSkins(const std::vector<const ufbx_node*>& sources, std::vector<ModelNode>& nodes) {
        std::unordered_map<uint32_t, int> indexByElementId;
        for (size_t i = 0; i < sources.size(); i++) indexByElementId[sources[i]->element_id] = (int)i;

        for (size_t i = 0; i < nodes.size(); i++) {
            if (!nodes[i].IsSkinned()) continue;
            const ufbx_skin_deformer* skin = sources[i]->mesh->skin_deformers.data[0];

            bool ok = true;
            for (size_t c = 0; c < skin->clusters.count && ok; c++) {
                const ufbx_skin_cluster* cluster = skin->clusters.data[c];
                auto found = cluster->bone_node ? indexByElementId.find(cluster->bone_node->element_id) : indexByElementId.end();
                if (found == indexByElementId.end()) { ok = false; break; }
                nodes[i].skinBones.push_back(found->second);

                // ufbxの列ベクトルの行列(3x4)を、DirectXの行ベクトル用の行列にする(行と列を入れ替える)
                const ufbx_matrix& m = cluster->geometry_to_bone;
                XMFLOAT4X4 inverseBind(
                    (float)m.m00, (float)m.m10, (float)m.m20, 0.f,
                    (float)m.m01, (float)m.m11, (float)m.m21, 0.f,
                    (float)m.m02, (float)m.m12, (float)m.m22, 0.f,
                    (float)m.m03, (float)m.m13, (float)m.m23, 1.f);
                nodes[i].skinInverseBind.push_back(inverseBind);
            }
            if (!ok) {
                OutputDebugStringA(("★ Model::Load: 骨のパーツが見つからないので、曲げずに静止のまま表示します: " + nodes[i].name + "\n").c_str());
                nodes[i].skinVertices.clear();
                nodes[i].skinBones.clear();
                nodes[i].skinInverseBind.clear();
            }
        }
    }

    bool LoadFbx(const std::wstring& path, std::vector<ModelNode>& nodes, std::vector<ModelClip>& clips) {
        std::vector<char> data;
        if (!ReadAllBytes(path, data)) {
            OutputDebugStringW((L"★ Model::Load: ファイルが開けません: " + path + L"\n").c_str());
            return false;
        }

        // 単位と座標系は、読み込む時にこのエンジンに合わせる
        //   単位: 1単位=1cm(FBXの中の単位設定から変換)
        //   座標系: Y上・左手系(DirectX)。右手系のMaya等からは、Zを反転する(三角形の頂点順もufbxが直す)
        ufbx_load_opts opts = {};
        opts.target_axes = ufbx_axes_left_handed_y_up;
        opts.target_unit_meters = 0.01;
        opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
        opts.handedness_conversion_axis = UFBX_MIRROR_AXIS_Z;
        opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
        opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
        opts.generate_missing_normals = true;

        ufbx_error error;
        ufbx_scene* scene = ufbx_load_memory(data.data(), data.size(), &opts, &error);
        if (!scene) {
            OutputDebugStringW((L"★ Model::Load: FBXを読めませんでした: " + path + L"\n").c_str());
            OutputDebugStringA(("  理由: " + std::string(error.description.data, error.description.length) + "\n").c_str());
            return false;
        }

        // 一番上の仮のノード(root)は含めず、その子から順に並べる
        std::vector<const ufbx_node*> sources;   // nodesと同じ並びの、ufbx側のノード
        for (size_t i = 0; i < scene->root_node->children.count; i++) {
            AddNode(scene->root_node->children.data[i], -1, nodes, sources);
        }
        FinishSkins(sources, nodes);
        BuildClips(scene, sources, clips);
        ufbx_free_scene(scene);
        return true;
    }
}

Model::~Model() {
    // 描画用バッファを返す(Meshは自分で解放する決まり)
    for (auto& mesh : meshes) {
        if (mesh) mesh->Uninit();
    }
    meshes.clear();
    for (auto& skinMesh : skinMeshes) {
        if (skinMesh) skinMesh->Uninit();
    }
    skinMeshes.clear();
}

std::shared_ptr<Model> Model::Load(const std::wstring& path) {
    auto model = std::make_shared<Model>();
    model->path = path;

    std::wstring ext = GetExtension(path);
    if (ext == L".fbx") {
        if (!LoadFbx(path, model->nodes, model->clips)) return nullptr;
    }
    else if (ext == L".obj") {
        // OBJは「パーツ1つだけのモデル」として読む
        ModelNode node;
        node.name = "Root";
        node.vertices = Mesh::LoadOBJ(path);
        if (node.vertices.empty()) return nullptr;
        model->nodes.push_back(std::move(node));
    }
    else {
        OutputDebugStringW((L"★ Model::Load: 対応していない形式です(.fbx か .obj): " + path + L"\n").c_str());
        return nullptr;
    }

    if (model->nodes.empty()) {
        OutputDebugStringW((L"★ Model::Load: パーツが1つもありません: " + path + L"\n").c_str());
        return nullptr;
    }
    model->CalculateBoundingRadius();
    return model;
}

void Model::CalculateBoundingRadius() {
    // パーツごとの「モデル空間」の行列(親が必ず先に並んでいる)で、全頂点の原点からの距離の最大を求める
    std::vector<XMMATRIX> matrices(nodes.size());
    float maxLengthSq = 0.f;
    for (size_t i = 0; i < nodes.size(); i++) {
        Transform local;
        local.position = nodes[i].position;
        local.rotate = nodes[i].rotate;
        local.scale = nodes[i].scale;
        matrices[i] = local.GetLocalMatrix();
        if (nodes[i].parent >= 0) matrices[i] = matrices[i] * matrices[nodes[i].parent];

        // 形が無いパーツ(骨など)も、パーツ自身の位置までは入れる
        XMFLOAT3 origin;
        XMStoreFloat3(&origin, matrices[i].r[3]);
        maxLengthSq = (std::max)(maxLengthSq, origin.x * origin.x + origin.y * origin.y + origin.z * origin.z);

        for (const MeshVertex& v : nodes[i].vertices) {
            XMFLOAT3 p;
            XMStoreFloat3(&p, XMVector3Transform(XMLoadFloat3(&v.Position), matrices[i]));
            maxLengthSq = (std::max)(maxLengthSq, p.x * p.x + p.y * p.y + p.z * p.z);
        }
    }
    boundingRadius = sqrtf(maxLengthSq);
}

int Model::FindClip(const std::string& name) const {
    for (int i = 0; i < (int)clips.size(); i++) {
        if (clips[i].name == name) return i;
    }
    return -1;
}

int Model::FindNode(const std::string& name) const {
    for (int i = 0; i < (int)nodes.size(); i++) {
        if (nodes[i].name == name) return i;
    }
    return -1;
}

Mesh* Model::GetMesh(int index) {
    if (index < 0 || index >= (int)nodes.size() || nodes[index].vertices.empty()) return nullptr;

    if (meshes.size() != nodes.size()) {
        meshes.resize(nodes.size());
        meshFailed.assign(nodes.size(), false);
    }
    if (meshFailed[index]) return nullptr;

    // 初めて描く時に、GPUのバッファを作る(以降はみんなで使い回す)
    if (!meshes[index]) {
        auto mesh = std::make_shared<Mesh>();
        if (!mesh->Init(nodes[index].vertices)) {
            OutputDebugStringW((L"★ Model::GetMesh: 描画用バッファを作れませんでした: " + path + L"\n").c_str());
            meshFailed[index] = true;
            return nullptr;
        }
        meshes[index] = mesh;
    }
    return meshes[index].get();
}

SkinMesh* Model::GetSkinMesh(int index) {
    if (index < 0 || index >= (int)nodes.size() || !nodes[index].IsSkinned()) return nullptr;

    if (skinMeshes.size() != nodes.size()) {
        skinMeshes.resize(nodes.size());
        skinFailed.assign(nodes.size(), false);
    }
    if (skinFailed[index]) return nullptr;

    // 初めて描く時に、GPUのバッファ(変更不可)を作る。以降はみんなで使い回す
    if (!skinMeshes[index]) {
        auto skinMesh = std::make_shared<SkinMesh>();
        if (!skinMesh->Init(nodes[index].skinVertices)) {
            OutputDebugStringW((L"★ Model::GetSkinMesh: スキンメッシュを作れなかったので、静止のメッシュで表示します: " + path + L"\n").c_str());
            skinFailed[index] = true;
            return nullptr;
        }
        skinMeshes[index] = skinMesh;
    }
    return skinMeshes[index].get();
}