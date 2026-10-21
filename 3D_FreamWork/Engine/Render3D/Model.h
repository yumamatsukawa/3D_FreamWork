#pragma once
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include "Mesh.h"
#include "SkinMesh.h"

// モデルのパーツ(ノード)1つぶんのデータ。親子関係を保ったまま持つ
struct ModelNode {
    std::string name;                        // Mayaで付けた名前
    int parent = -1;                         // 親のパーツの番号(一番上のパーツは-1)。親は必ず自分より小さい番号
    XMFLOAT3 position{};                     // 親からの相対位置(1単位=1cm)
    XMFLOAT3 rotate{};                       // 親からの相対回転(度数。Transformと同じ向き)
    XMFLOAT3 scale{ 1.f, 1.f, 1.f };         // 親からの相対スケール
    std::vector<MeshVertex> vertices;        // 形(三角形の頂点。パーツの中心が原点。形が無いパーツは空)
    std::wstring texturePath;                // このパーツの画像(無ければ空)
    XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };    // マテリアルのdiffuse色(マテリアルが無い・色が無い・画像のマテリアルは白。アルファは常に1)
    // ─── 骨で曲げるメッシュの時だけ入る(スキンが無ければskinVerticesは空) ───
    std::vector<SkinVertex> skinVertices;    // 骨の番号・影響度つきの頂点(verticesと同じ並び・同じ数)。骨は最大4本、影響度は合計1に直してある
    std::vector<int> skinBones;              // このメッシュが使う骨のパーツの番号(skinVerticesのBoneIndicesは、このリストの何番目か)
    std::vector<XMFLOAT4X4> skinInverseBind; // 骨ごとの逆バインド行列(メッシュの頂点 → 骨の空間。skinBonesと同じ並び)
    bool IsSkinned() const { return !skinVertices.empty(); }
};

// パーツ1つの、ある時刻の位置・回転・スケール(親のパーツからの相対)
struct ModelPose {
    XMFLOAT3 position{};
    XMFLOAT4 rotation{ 0.f, 0.f, 0.f, 1.f };   // クォータニオン(x, y, z, w)
    XMFLOAT3 scale{ 1.f, 1.f, 1.f };
};

// アニメーションのクリップ1つ(Mayaで付けた名前の動き)。
// 読み込む時に、全パーツの動きを一定の細かさ(fps)で並べ直して持つ。位置は cm・左手系で、静止モデルと同じ変換を済ませてある
struct ModelClip {
    std::string name;
    float duration = 0.f;                          // 長さ(秒)
    float fps = 30.f;                              // 1秒に何個の値を持っているか
    int frameCount = 1;                            // 値の個数(0秒〜長さ。両端を含む)
    std::vector<std::vector<ModelPose>> nodePoses; // [パーツの番号][何個目の値]
};

// 読み込んだモデル(.fbx / .obj)。みんなで共有するデータで、ゲーム側は普通 ModelRenderer 経由で使う。
// 形と親子関係だけを持ち、パーツの今の動き(位置・回転)は ModelRenderer が1体ごとに持つ
class Model {
private:
    std::vector<ModelNode> nodes;
    std::wstring path;
    std::vector<ModelClip> clips;
    std::vector<std::shared_ptr<Mesh>> meshes;   // 描画用のバッファ(最初に描く時に作る。みんなで共有)
    std::vector<std::shared_ptr<SkinMesh>> skinMeshes;   // スキンメッシュの描画用バッファ(最初に描く時に作る)
    std::vector<bool> skinFailed;
    std::vector<bool> meshFailed;                // バッファを作れなかったパーツ(毎フレーム作り直さないため)
    float boundingRadius = 0.f;                  // モデルの原点から、止まっている姿の一番遠い頂点までの距離

    void CalculateBoundingRadius();              // boundingRadiusを求める(読み込んだ後に1回)

public:
    ~Model();

    // .fbx / .obj を読む。失敗したらnullptr(出力ウィンドウに理由を出す)
    // 同じファイルを何度も読まないように、ゲーム側は普通 Resources::LoadModel を使う
    static std::shared_ptr<Model> Load(const std::wstring& path);

    int GetNodeCount() const { return (int)nodes.size(); }
    // 名前でパーツを探す(無ければ-1)
    int FindNode(const std::string& name) const;
    const ModelNode& GetNode(int index) const { return nodes[index]; }
    // パーツのマテリアルのdiffuse色(読み取り専用。ModelRenderer が描く時に、SetColor の色へ掛ける)。範囲外は白
    XMFLOAT4 GetNodeColor(int index) const {
        return (index >= 0 && index < (int)nodes.size()) ? nodes[index].color : XMFLOAT4{ 1.f, 1.f, 1.f, 1.f };
    }
    const std::wstring& GetPath() const { return path; }
    // モデルの原点から、止まっている姿の一番遠い頂点までの距離(画面判定の球の半径。ModelRendererが使う)。
    // 骨・アニメーションで動くとはみ出すので、使う側で余裕を持たせる
    float GetBoundingRadius() const { return boundingRadius; }

    // アニメーションのクリップ(ModelAnimationComponent が使う)。FindClip は無ければ-1
    int GetClipCount() const { return (int)clips.size(); }
    const ModelClip& GetClip(int index) const { return clips[index]; }
    int FindClip(const std::string& name) const;

    // クォータニオンを、Transform::rotate と同じ向きのオイラー角(度)にする
    static XMFLOAT3 QuaternionToEuler(const XMFLOAT4& q);

    // パーツ1つぶんの描画用バッファ(形が無いパーツはnullptr)。Engine内部用(ModelRendererが使う)
    Mesh* GetMesh(int index);
    // 骨で曲げるパーツの描画用バッファ(スキンが無い・作れなかったパーツはnullptr)。Engine内部用(ModelRendererが使う)
    SkinMesh* GetSkinMesh(int index);
};
