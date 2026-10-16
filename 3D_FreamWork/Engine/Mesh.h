#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#pragma comment(lib, "d3dcompiler.lib")

struct MeshVertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT4 Color;
    XMFLOAT2 TexCoord;
};

struct MeshConstantBuffer {
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX proj;
    XMFLOAT4 lightDir;
    XMFLOAT4 lightColor;
    XMFLOAT4 ambient;
};

// カメラの位置(Shader3DCommon.hlsli の CameraBuffer, b3)。光沢の計算で「ピクセルからカメラへの向き」を求めるのに使う
struct MeshCameraBuffer {
    XMFLOAT4 eyePos;   // xyz=カメラのワールド座標
};

// カプセルの長い向き(PhysXは元々X軸だが、立って歩く形になるよう、既定はYにする)
enum class CapsuleAxis { X, Y, Z };

// 3Dの頂点データ(立体)を持ち、World/View/Projection行列を使って描画するクラス。
// SpriteがZ軸回転のみ・奥行き無しの2D専用だったのに対し、こちらはXYZ全軸の回転や
// 奥行きのある立体を、深度バッファ有り(前後関係が正しい)で描画できる。
class Mesh {
private:
    // インスタンスごとに違うもの(形状データ・テクスチャ)。
    // 頂点バッファ(とインデックスバッファ)は作る時に1回だけ作り、描く時に毎回コピーしない(変更不可)
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;   // InitIndexedで作った時だけ。nullptrなら頂点を順に描く
    ID3D11ShaderResourceView* srv = nullptr;
    UINT vertexCount = 0;
    UINT indexCount = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // シェーダー・各種ステート・定数バッファは、Cube/Sphere/OBJなど全てのMeshで
    // 内容が同じ(Shader3D.hlslを共有している)ため、インスタンスごとに作らず
    // クラス全体で1つだけ作って使い回す(コンパイル・生成コストの削減)。
    // refCountで「今何体のMeshが使っているか」を数え、最後の1体がUninitされた時に
    // まとめて解放する
    static ID3D11DeviceContext* context;
    static ID3D11Buffer* constantBuffer;     // b0: MeshConstantBuffer
    static ID3D11Buffer* materialBuffer;     // b2: MaterialConstantBuffer
    static ID3D11Buffer* cameraBuffer;       // b3: MeshCameraBuffer
    static ID3D11VertexShader* vertexShader;
    static ID3D11PixelShader* pixelShader;
    static ID3D11InputLayout* inputLayout;
    static ID3D11SamplerState* sampler;
    static ID3D11BlendState* blendState;
    static ID3D11DepthStencilState* depthStencilState;
    static ID3D11DepthStencilState* depthStencilStateNoWrite; // スカイボックス用: テストはするが書き込みしない
    static ID3D11RasterizerState* rasterizerState;
    // テクスチャを設定していないMesh用の、1x1の白いテクスチャ。
    // シェーダーは必ずテクスチャを読むので、何も渡さないと「直前に別のMeshが使った
    // テクスチャ」を読んでしまい、色が変わったり透明部分で消えたりする
    static ID3D11ShaderResourceView* whiteTexture;
    static float aspectRatio;
    static int refCount;

    bool CreateSharedResources();
    static void ReleaseSharedResources();
    static bool CreateShaders();
    static bool CreateConstantBuffer();
    static bool CreateSampler();
    static bool CreateBlendState();
    static bool CreateDepthStencilState();
    static bool CreateRasterizerState();
    static bool CreateWhiteTexture();

    bool CreateBuffers(const std::vector<MeshVertex>& verts, const std::vector<UINT>* indices);
    bool InitInternal(const std::vector<MeshVertex>& verts, const std::vector<UINT>* indices, D3D11_PRIMITIVE_TOPOLOGY topology);

public:
    // topology: 通常は三角形(TRIANGLELIST)。Create○○Outline()の線データを渡す時は
    // D3D11_PRIMITIVE_TOPOLOGY_LINELIST(2頂点ずつ1本の線)にする
    // 頂点バッファは、ここで1回だけ作る(変更不可)。失敗したらfalse(その時はUninitを呼ばない)
    bool Init(const std::vector<MeshVertex>& verts,
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // インデックス(頂点の番号)を使う版。三角形3つぶんの番号を並べる(TRIANGLELISTなら3つで1枚、LINELISTなら2つで1本)。
    // 同じ頂点を使い回せるので、地形のように頂点が多い形は頂点の数が大きく減る
    bool InitIndexed(const std::vector<MeshVertex>& verts, const std::vector<UINT>& indices,
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // マテリアル(色・光沢・自発光・光を使うか・深度を書くか)を指定して描く。
    // material.depthWrite = false にすると、深度バッファに書き込まない(自分より後ろのものを隠さない)。
    // スカイボックス用: drawPriorityで必ず最初に描画させた上でこれをfalseにすることで、
    // 「深度テストで他の3Dオブジェクトと競合する遠景」ではなく「常に一番奥にある背景」として
    // 描画できる(深度バッファの精度に依存しない、Z-fightingが起こり得ない実装)。
    // ※material.texture(パス)はここでは使わない。画像は SetTexture で渡す(MeshRenderer / ModelRenderer が読んで渡す)
    void Draw(const Transform& transform, const Camera& camera, const Material& material, const Light& light);
    // ワールド行列を直接渡す版(ModelRendererが、パーツごとに累積した行列で描く時に使う)
    void Draw(const XMMATRIX& world, const Camera& camera, const Material& material, const Light& light);

    // 色だけ指定する版(既定のマテリアルで描く)。writeDepth = false で深度に書き込まない
    void Draw(const Transform& transform, const Camera& camera, XMFLOAT4 color, const Light& light, bool writeDepth = true);
    void Draw(const XMMATRIX& world, const Camera& camera, XMFLOAT4 color, const Light& light, bool writeDepth = true);
    void Uninit();

    void SetTexture(ID3D11ShaderResourceView* s) { srv = s; }

    // 単位立方体(-0.5〜0.5)の頂点データを作る。obj->transform.scaleで大きさを変えられる
    static std::vector<MeshVertex> CreateCube();

    // 半径0.5の単位球の頂点データを作る。obj->transform.scaleで大きさを変えられる。
    // rings: 緯度方向の分割数、segments: 経度方向の分割数(大きいほど滑らかで頂点数が増える)
    static std::vector<MeshVertex> CreateSphere(int rings = 16, int segments = 24);

    // カプセル(円柱の両端が半球)の頂点データを作る。長い向きはY軸。
    // 立方体や球と違って「大きさ1の形をscaleで伸ばす」と両端の半球が潰れてしまうので、
    // 実際の大きさで作る(使う時のtransform.scaleは1のままにする)。
    // radius: 半径、height: 全体の高さ(両端の半球を含む。2×radiusより小さい時は球になる)
    // rings: 半球1つの緯度方向の分割数、segments: 経度方向の分割数
    static std::vector<MeshVertex> CreateCapsule(float radius, float height, int rings = 8, int segments = 24);

    // Wavefront OBJ(.obj)ファイルを読み込んで頂点データを作る。
    // v(頂点座標)/vt(UV座標)/vn(法線)/f(面)に対応。四角形以上の面は三角形に自動分割する。
    // vnが無いファイルは、三角形ごとに面法線を自動計算する。
    // 読み込みに失敗した場合は空のvectorを返す
    static std::vector<MeshVertex> LoadOBJ(const std::wstring& filepath);

    // ─── 輪郭(線)だけのデータ。Collider表示などのデバッグ用 ───
    // どれもInit(verts, D3D11_PRIMITIVE_TOPOLOGY_LINELIST)と組み合わせて使う。大きさは1(-0.5〜0.5)

    // 立方体の12本の辺
    static std::vector<MeshVertex> CreateCubeOutline();
    // XY平面上(z=0)の正方形の4辺
    static std::vector<MeshVertex> CreateSquareOutline();
    // XY平面上(z=0)の直径1の円。segmentsは円を何本の線で近似するか
    static std::vector<MeshVertex> CreateCircleOutline(int segments = 32);
    // 直径1の球を表す3つの円(XY/XZ/YZ平面)
    static std::vector<MeshVertex> CreateSphereOutline(int segments = 32);
    // カプセルの輪郭。ここだけは大きさ1ではなく、実際の大きさ(radius=半径、halfHeight=円柱部分の長さの半分)で作る。
    // axis: 長い向き。crossSectionOnly=trueなら、XY平面で切った断面(角の丸い四角=スタジアム形)だけを作る(2D用)
    static std::vector<MeshVertex> CreateCapsuleOutline(float radius, float halfHeight,
        CapsuleAxis axis = CapsuleAxis::Y, int segments = 24, bool crossSectionOnly = false);
};
