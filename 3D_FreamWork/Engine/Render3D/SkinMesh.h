#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "Mesh.h"

// 骨で曲げるメッシュの頂点(MeshVertex に、骨の番号と影響度を足した形)
struct SkinVertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT4 Color;
    XMFLOAT2 TexCoord;
    UINT BoneIndices[4];     // 影響する骨の番号(そのメッシュの骨のリストの何番目か)。影響が4本未満なら残りは0
    XMFLOAT4 BoneWeights;    // 影響度(合計1。残りは0)
};

// 骨の行列で頂点を動かして描く3Dメッシュ(Shader3DSkin.hlsl)。Mesh のスキン版。
// 頂点バッファは読み込み時に1回だけ作り(変更不可)、毎フレームは骨の行列だけを送る。
// シェーダー・ステート・定数バッファは Mesh と同じく、全SkinMeshで1つを共有し、最後の1体がUninitされた時に解放する
class SkinMesh {
public:
    static constexpr int maxBones = 64;   // 1つのメッシュに使える骨の数の上限(Shader3DSkin.hlslの MAX_BONES と同じ)

private:
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* smoothNormalBuffer = nullptr;   // 枠線用: 同じ位置の頂点の法線を平均した法線
    ID3D11ShaderResourceView* srv = nullptr;
    UINT vertexCount = 0;

    static ID3D11DeviceContext* context;
    static ID3D11Buffer* constantBuffer;     // b0: Shader3D.hlsl と同じ(MeshConstantBuffer)
    static ID3D11Buffer* skinBuffer;         // b1: 骨の行列
    static ID3D11Buffer* materialBuffer;     // b2: MaterialConstantBuffer(Mesh と同じ)
    static ID3D11Buffer* cameraBuffer;       // b3: MeshCameraBuffer(Mesh と同じ。b4 の光は Lighting::Bind())
    static ID3D11VertexShader* vertexShader;
    static ID3D11PixelShader* pixelShader;
    static ID3D11VertexShader* shadowVertexShader;   // 影の深度パス用(Shader3DSkin.hlsl の VSShadow。ピクセルシェーダー無し)
    static ID3D11VertexShader* outlineVertexShader;  // 枠線用(Shader3DSkin.hlsl の VSOutline)
    static ID3D11PixelShader* outlinePixelShader;    // 枠線用(Shader3DSkin.hlsl の PSOutline)
    static ID3D11InputLayout* inputLayout;
    static ID3D11InputLayout* outlineInputLayout;    // 枠線用: 位置・法線・骨(slot 0)+平均した法線(slot 1)
    static ID3D11SamplerState* sampler;
    static ID3D11BlendState* blendState;
    static ID3D11DepthStencilState* depthStencilState;
    static ID3D11DepthStencilState* depthStencilStateNoWrite;   // material.depthWrite = false の時: テストはするが書き込みしない
    static ID3D11DepthStencilState* outlineMarkState;   // 枠線用: 本物の見えている画素のステンシルを書き換える(深度は書かない)
    static ID3D11DepthStencilState* outlineMarkAllState;          // 枠線(壁越し): 深度テストなしで、シルエット全体のステンシルを書き換える
    static ID3D11DepthStencilState* outlineRingSeeThroughState;   // 枠線(壁越し): 深度テストなしで、シルエットの外側だけに描く
    static ID3D11DepthStencilState* outlineRingState;   // 枠線用: ステンシルが1の画素(本物の内側)には描かない
    static ID3D11RasterizerState* rasterizerState;
    static ID3D11ShaderResourceView* whiteTexture;   // テクスチャが無い時に使う1x1の白
    static float aspectRatio;
    static int refCount;

    static bool CreateSharedResources();
    static void ReleaseSharedResources();
    static bool CreateShaders();
    static bool CreateStates();

public:
    // 頂点バッファを作る。失敗したらfalse(出力ウィンドウに理由が出る。その時はUninitを呼ばない)
    bool Init(const std::vector<SkinVertex>& verts);
    void Uninit();

    void SetTexture(ID3D11ShaderResourceView* s) { srv = s; }

    // boneMatrices: スキン行列(逆バインド行列 × 骨のモデル空間の行列)。boneCountは maxBones まで
    // world: モデル空間 → ワールド空間の行列(オーナーのワールド行列)
    // material: 色・光沢・自発光・光を使うか・深度を書くか(material.texture のパスはここでは使わない。画像は SetTexture で渡す)
    void Draw(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
        const Camera& camera, const Material& material, const Light& light);

    // 色だけ指定する版(既定のマテリアルで描く)
    void Draw(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
        const Camera& camera, XMFLOAT4 color, const Light& light);

    // 枠線を描く(本物を Draw した直後に、同じ行列・骨で呼ぶ)。Mesh::DrawOutline と同じ仕組みで、骨で曲げた後の形の外側に、画面上で widthPixels ピクセルの太さの線を描く。
    // widthPixels <= 0 や color.w <= 0 では何もしない。material.depthWrite = false の物には使わない。
    // seeThrough = true の時は、Mesh::DrawOutline と同じく、手前の物に隠れていても枠線の輪を描く(物の drawPriority を大きくしておく)
    void DrawOutline(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
        const Camera& camera, XMFLOAT4 color, float widthPixels, bool seeThrough = false);

    // 影のパス用: 骨で曲げた後の形を、光から見た行列(lightView / lightProj)で、深度だけ描く(色は描かない)。
    // 描画先・ビューポート・ステートは Shadow::Render が決める
    void DrawDepth(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
        const XMMATRIX& lightView, const XMMATRIX& lightProj);
};
